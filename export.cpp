#include "solvespace.h"
#include <png.h>

void SolveSpace::ExportSectionTo(char *filename) {
    SPolygon sp;
    ZERO(&sp);

    Vector gn = (SS.GW.projRight).Cross(SS.GW.projUp);
    gn = gn.WithMagnitude(1);

    Group *g = SS.GetGroup(SS.GW.activeGroup);
    if(g->runningMesh.l.n == 0) {
        Error("No solid model present; draw one with extrudes and revolves, "
              "or use Export 2d View to export bare lines and curves.");
        return;
    }
    
    // The plane in which the exported section lies; need this because we'll
    // reorient from that plane into the xy plane before exporting.
    Vector origin, u, v, n;
    double d;

    SS.GW.GroupSelection();
#define gs (SS.GW.gs)
    if((gs.n == 0 && g->activeWorkplane.v != Entity::FREE_IN_3D.v)) {
        Entity *wrkpl = SS.GetEntity(g->activeWorkplane);
        origin = wrkpl->WorkplaneGetOffset();
        n = wrkpl->Normal()->NormalN();
        u = wrkpl->Normal()->NormalU();
        v = wrkpl->Normal()->NormalV();
    } else if(gs.n == 1 && gs.faces == 1) {
        Entity *face = SS.GetEntity(gs.entity[0]);
        origin = face->FaceGetPointNum();
        n = face->FaceGetNormalNum();
        if(n.Dot(gn) < 0) n = n.ScaledBy(-1);
        u = n.Normal(0);
        v = n.Normal(1);
    } else if(gs.n == 3 && gs.vectors == 2 && gs.points == 1) {
        Vector ut = SS.GetEntity(gs.entity[0])->VectorGetNum(),
               vt = SS.GetEntity(gs.entity[1])->VectorGetNum();
        ut = ut.WithMagnitude(1);
        vt = vt.WithMagnitude(1);

        if(fabs(SS.GW.projUp.Dot(vt)) < fabs(SS.GW.projUp.Dot(ut))) {
            SWAP(Vector, ut, vt);
        }
        if(SS.GW.projRight.Dot(ut) < 0) ut = ut.ScaledBy(-1);
        if(SS.GW.projUp.   Dot(vt) < 0) vt = vt.ScaledBy(-1);

        origin = SS.GetEntity(gs.point[0])->PointGetNum();
        n = ut.Cross(vt);
        u = ut.WithMagnitude(1);
        v = (n.Cross(u)).WithMagnitude(1);
    } else {
        Error("Bad selection for export section. Please select:\r\n\r\n"
              "    * nothing, with an active workplane "
                        "(workplane is section plane)\r\n"
              "    * a face (section plane through face)\r\n"
              "    * a point and two line segments "
                        "(plane through point and parallel to lines)\r\n");
        return;
    }
    SS.GW.ClearSelection();

    n = n.WithMagnitude(1);
    d = origin.Dot(n);

    SMesh m;
    ZERO(&m);
    m.MakeFromCopy(&(g->runningMesh));

    // Delete all triangles in the mesh that do not lie in our export plane.
    m.l.ClearTags();
    int i;
    for(i = 0; i < m.l.n; i++) {
        STriangle *tr = &(m.l.elem[i]);

        if((fabs(n.Dot(tr->a) - d) >= LENGTH_EPS) ||
           (fabs(n.Dot(tr->b) - d) >= LENGTH_EPS) ||
           (fabs(n.Dot(tr->c) - d) >= LENGTH_EPS))
        {
            tr->tag  = 1;
        }
    }
    m.l.RemoveTagged();

    // Select the naked edges in our resulting open mesh.
    SKdNode *root = SKdNode::From(&m);
    SEdgeList el;
    ZERO(&el);
    root->MakeNakedEdgesInto(&el);
    // Assemble those edges into a polygon, and clear the edge list
    el.AssemblePolygon(&sp, NULL);
    el.Clear();
    m.Clear();

    // And write the polygon.
    VectorFileWriter *out = VectorFileWriter::ForFile(filename);
    if(out) {
        ExportPolygon(&sp, u, v, n, origin, out);
    }
    sp.Clear();
}

void SolveSpace::ExportViewTo(char *filename) {
    int i;
    SEdgeList edges;
    ZERO(&edges);
    for(i = 0; i < SS.entity.n; i++) {
        Entity *e = &(SS.entity.elem[i]);
        if(!e->IsVisible()) continue;

        e->GenerateEdges(&edges);
    }
    
    SPolygon sp;
    ZERO(&sp);
    edges.AssemblePolygon(&sp, NULL);

    Vector u = SS.GW.projRight,
           v = SS.GW.projUp,
           n = u.Cross(v),
           origin = SS.GW.offset.ScaledBy(-1);

    VectorFileWriter *out = VectorFileWriter::ForFile(filename);
    if(out) {
        ExportPolygon(&sp, u, v, n, origin, out);
    }
    edges.Clear();
    sp.Clear();
}

void SolveSpace::ExportPolygon(SPolygon *sp,
                               Vector u, Vector v, Vector n, Vector origin,
                               VectorFileWriter *out)
{
    int i, j;
    // Project into the export plane; so when we're done, z doesn't matter,
    // and x and y are what goes in the DXF.
    for(i = 0; i < sp->l.n; i++) {
        SContour *sc = &(sp->l.elem[i]);
        for(j = 0; j < sc->l.n; j++) {
            Vector *p = &(sc->l.elem[j].p);
            *p = p->Minus(origin);
            *p = p->DotInToCsys(u, v, n);
            // and apply the export scale factor
            double s = SS.exportScale;
            *p = p->ScaledBy(1.0/s);
        }
    }

    // If cutter radius compensation is requested, then perform it now.
    if(fabs(SS.exportOffset) > LENGTH_EPS) {
        SPolygon compd;
        ZERO(&compd);
        sp->normal = Vector::From(0, 0, -1);
        sp->FixContourDirections();
        sp->OffsetInto(&compd, SS.exportOffset);
        sp->Clear();
        *sp = compd;
    }

    // Now begin the entities, which are just line segments reproduced from
    // our piecewise linear curves.
    out->OutputPolygon(sp);
}

bool VectorFileWriter::StringEndsIn(char *str, char *ending) {
    int i, ls = strlen(str), le = strlen(ending);

    if(ls < le) return false;
        
    for(i = 0; i < le; i++) {
        if(tolower(ending[le-i-1]) != tolower(str[ls-i-1])) {
            return false;
        }
    }
    return true;
}

VectorFileWriter *VectorFileWriter::ForFile(char *filename) {
    VectorFileWriter *ret;
    if(StringEndsIn(filename, ".dxf")) {
        static DxfFileWriter DxfWriter;
        ret = &DxfWriter;
    } else if(StringEndsIn(filename, ".ps") || StringEndsIn(filename, ".eps")) {
        static EpsFileWriter EpsWriter;
        ret = &EpsWriter;
    } else if(StringEndsIn(filename, ".svg")) {
        static SvgFileWriter SvgWriter;
        ret = &SvgWriter;
    } else if(StringEndsIn(filename, ".plt")||StringEndsIn(filename, ".hpgl")) {
        static HpglFileWriter HpglWriter;
        ret = &HpglWriter;
    } else {
        Error("Can't identify output file type from file extension of "
        "filename '%s'; try .dxf, .svg, .plt, .hpgl, .eps, or .ps.", filename);
        return NULL;
    }

    FILE *f = fopen(filename, "wb");
    if(!f) {
        Error("Couldn't write to '%s'", filename);
        return NULL;
    }
    ret->f = f;
    return ret;
}

void VectorFileWriter::OutputPolygon(SPolygon *sp) {
    int i, j;

    // First calculate the bounding box.
    ptMin = Vector::From(VERY_POSITIVE, VERY_POSITIVE, VERY_POSITIVE);
    ptMax = Vector::From(VERY_NEGATIVE, VERY_NEGATIVE, VERY_NEGATIVE);
    for(i = 0; i < sp->l.n; i++) {
        SContour *sc = &(sp->l.elem[i]);
        for(j = 0; j < sc->l.n; j++) {
            (sc->l.elem[j].p).MakeMaxMin(&ptMax, &ptMin);
        }
    }

    StartFile();
    for(i = 0; i < sp->l.n; i++) {
        SContour *sc = &(sp->l.elem[i]);

        for(j = 1; j < sc->l.n; j++) {
            Vector p0 = sc->l.elem[j-1].p,
                   p1 = sc->l.elem[j].p;

            LineSegment(p0.x, p0.y, p1.x, p1.y);
        }
    }
    FinishAndCloseFile();
}

//-----------------------------------------------------------------------------
// Routines for DXF export
//-----------------------------------------------------------------------------
void DxfFileWriter::StartFile(void) {
    // Some software, like Adobe Illustrator, insists on a header.
    fprintf(f,
"  999\r\n"
"file created by SolveSpace\r\n"
"  0\r\n"
"SECTION\r\n"
"  2\r\n"
"HEADER\r\n"
"  9\r\n"
"$ACADVER\r\n"
"  1\r\n"
"AC1006\r\n"
"  9\r\n"
"$INSBASE\r\n"
"  10\r\n"
"0.0\r\n"
"  20\r\n"
"0.0\r\n"
"  30\r\n"
"0.0\r\n"
"  9\r\n"
"$EXTMIN\r\n"
"  10\r\n"
"0.0\r\n"
"  20\r\n"
"0.0\r\n"
"  9\r\n"
"$EXTMAX\r\n"
"  10\r\n"
"10000.0\r\n"
"  20\r\n"
"10000.0\r\n"
"  0\r\n"
"ENDSEC\r\n");

    // Then start the entities.
    fprintf(f,
"  0\r\n"
"SECTION\r\n"
"  2\r\n"
"ENTITIES\r\n");
}

void DxfFileWriter::LineSegment(double x0, double y0, double x1, double y1) {
    fprintf(f,
"  0\r\n"
"LINE\r\n"
"  8\r\n"     // Layer code
"%d\r\n"
"  10\r\n"    // xA
"%.6f\r\n"
"  20\r\n"    // yA
"%.6f\r\n"
"  30\r\n"    // zA
"%.6f\r\n"
"  11\r\n"    // xB
"%.6f\r\n"
"  21\r\n"    // yB
"%.6f\r\n"
"  31\r\n"    // zB
"%.6f\r\n",
                    0,
                    x0, y0, 0.0,
                    x1, y1, 0.0);
}

void DxfFileWriter::FinishAndCloseFile(void) {
    fprintf(f,
"  0\r\n"
"ENDSEC\r\n"
"  0\r\n"
"EOF\r\n" );
    fclose(f);
}

//-----------------------------------------------------------------------------
// Routines for EPS output
//-----------------------------------------------------------------------------
double EpsFileWriter::MmToPoints(double mm) {
    // 72 points in an inch
    return (mm/25.4)*72;
}

void EpsFileWriter::StartFile(void) {
    fprintf(f,
"%%!PS-Adobe-2.0\r\n"
"%%%%Creator: SolveSpace\r\n"
"%%%%Title: title\r\n"
"%%%%Pages: 0\r\n"
"%%%%PageOrder: Ascend\r\n"
"%%%%BoundingBox: 0 0 %d %d\r\n"
"%%%%HiResBoundingBox: 0 0 %.3f %.3f\r\n"
"%%%%EndComments\r\n"
"\r\n"
"gsave\r\n"
"\r\n",
            (int)ceil(MmToPoints(ptMax.x - ptMin.x)),
            (int)ceil(MmToPoints(ptMax.y - ptMin.y)),
            MmToPoints(ptMax.x - ptMin.x),
            MmToPoints(ptMax.y - ptMin.y));
}

void EpsFileWriter::LineSegment(double x0, double y0, double x1, double y1) {
    fprintf(f,
"newpath\r\n"
"    %.3f %.3f moveto\r\n"
"    %.3f %.3f lineto\r\n"
"    1 setlinewidth\r\n"
"    0 setgray\r\n"
"stroke\r\n",
            MmToPoints(x0 - ptMin.x), MmToPoints(y0 - ptMin.y),
            MmToPoints(x1 - ptMin.x), MmToPoints(y1 - ptMin.y));
}

void EpsFileWriter::FinishAndCloseFile(void) {
    fprintf(f,
"\r\n"
"grestore\r\n"
"\r\n");
    fclose(f);
}

//-----------------------------------------------------------------------------
// Routines for SVG output
//-----------------------------------------------------------------------------
void SvgFileWriter::StartFile(void) {
    fprintf(f,
"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" "
    "\"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\r\n"
"<svg xmlns=\"http://www.w3.org/2000/svg\"  "
    "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
    "width='%.3fmm' height='%.3fmm' "
    "viewBox=\"0 0 %.3f %.3f\">\r\n"
"\r\n"
"<title>Exported SVG</title>\r\n"
"\r\n",
        ptMax.x - ptMin.x, ptMax.y - ptMin.y,
        ptMax.x - ptMin.x, ptMax.y - ptMin.y);
}

void SvgFileWriter::LineSegment(double x0, double y0, double x1, double y1) {
    fprintf(f,
"<polyline points='%.3f %.3f, %.3f %.3f' "
    "stroke-width='1' stroke='black' style='fill: none;' />\r\n",
            (x0 - ptMin.x), (y0 - ptMin.y),
            (x1 - ptMin.x), (y1 - ptMin.y));
}

void SvgFileWriter::FinishAndCloseFile(void) {
    fprintf(f, "\r\n</svg>\r\n");
    fclose(f);
}

//-----------------------------------------------------------------------------
// Routines for HPGL output
//-----------------------------------------------------------------------------
double HpglFileWriter::MmToHpglUnits(double mm) {
    return mm*40;
}

void HpglFileWriter::StartFile(void) {
    fprintf(f, "IN;\r\n");
    fprintf(f, "SP1;\r\n");
}

void HpglFileWriter::LineSegment(double x0, double y0, double x1, double y1) {
    fprintf(f, "PU%d,%d;\r\n", (int)MmToHpglUnits(x0), (int)MmToHpglUnits(y0));
    fprintf(f, "PD%d,%d;\r\n", (int)MmToHpglUnits(x1), (int)MmToHpglUnits(y1));
}

void HpglFileWriter::FinishAndCloseFile(void) {
    fclose(f);
}

//-----------------------------------------------------------------------------
// Export the mesh as an STL file; it should always be vertex-to-vertex and
// not self-intersecting, so not much to do.
//-----------------------------------------------------------------------------
void SolveSpace::ExportMeshTo(char *filename) {
    SMesh *m = &(SS.GetGroup(SS.GW.activeGroup)->runningMesh);
    if(m->l.n == 0) {
        Error("Active group mesh is empty; nothing to export.");
        return;
    }

    FILE *f = fopen(filename, "wb");
    if(!f) {
        Error("Couldn't write to '%s'", filename);
        return;
    }
    char str[80];
    memset(str, 0, sizeof(str));
    strcpy(str, "STL exported mesh");
    fwrite(str, 1, 80, f);

    DWORD n = m->l.n;
    fwrite(&n, 4, 1, f);

    double s = SS.exportScale;
    int i;
    for(i = 0; i < m->l.n; i++) {
        STriangle *tr = &(m->l.elem[i]);
        Vector n = tr->Normal().WithMagnitude(1);
        float w;
        w = (float)n.x;           fwrite(&w, 4, 1, f);
        w = (float)n.y;           fwrite(&w, 4, 1, f);
        w = (float)n.z;           fwrite(&w, 4, 1, f);
        w = (float)((tr->a.x)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->a.y)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->a.z)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->b.x)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->b.y)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->b.z)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->c.x)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->c.y)/s); fwrite(&w, 4, 1, f);
        w = (float)((tr->c.z)/s); fwrite(&w, 4, 1, f);
        fputc(0, f);
        fputc(0, f);
    }

    fclose(f);
}

//-----------------------------------------------------------------------------
// Export a view of the model as an image; we just take a screenshot, by
// rendering the view in the usual way and then copying the pixels.
//-----------------------------------------------------------------------------
void SolveSpace::ExportAsPngTo(char *filename) {
    int w = (int)SS.GW.width, h = (int)SS.GW.height;
    // No guarantee that the back buffer contains anything valid right now,
    // so repaint the scene. And hide the toolbar too.
    int prevShowToolbar = SS.showToolbar;
    SS.showToolbar = false;
    SS.GW.Paint(w, h);
    SS.showToolbar = prevShowToolbar;
    
    FILE *f = fopen(filename, "wb");
    if(!f) goto err;

    png_struct *png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        NULL, NULL, NULL);
    if(!png_ptr) goto err;

    png_info *info_ptr = png_create_info_struct(png_ptr);
    if(!png_ptr) goto err;

    if(setjmp(png_jmpbuf(png_ptr))) goto err;

    png_init_io(png_ptr, f);

    // glReadPixels wants to align things on 4-boundaries, and there's 3
    // bytes per pixel. As long as the row width is divisible by 4, all
    // works out.
    w &= ~3; h &= ~3;

    png_set_IHDR(png_ptr, info_ptr, w, h,
        8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
    
    png_write_info(png_ptr, info_ptr);

    // Get the pixel data from the framebuffer
    BYTE *pixels = (BYTE *)AllocTemporary(3*w*h);
    BYTE **rowptrs = (BYTE **)AllocTemporary(h*sizeof(BYTE *));
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    int y;
    for(y = 0; y < h; y++) {
        // gl puts the origin at lower left, but png puts it top left
        rowptrs[y] = pixels + ((h - 1) - y)*(3*w);
    }
    png_write_image(png_ptr, rowptrs);

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(f);
    return;

err:    
    Error("Error writing PNG file '%s'", filename);
    if(f) fclose(f);
    return;
}

