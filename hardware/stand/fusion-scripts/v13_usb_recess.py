# v13 = v12, USB-C: the 1.5 mm panel for the clips now comes from an OUTER recess shaped like the socket flange
# (flange sits flush-ish instead of standing out), the inner 20 x 7 pocket is filled again (user 14.10).
import adsk.core, adsk.fusion, math, os
exec(open(r'D:\___Claude\_Pub\Wortuhr\entwicklung\fusion_v10\edges_dump.py', encoding='utf-8').read().split('\ndef convex_info')[0])
FL_N, FL_U, CLR, FL_R = 15.5, 9.0, 0.3, 2.5     # flange (photo: 15.5 wide; height 9 from the first spec), clearance, corner R
REC_N, REC_U = FL_N + 2 * CLR, FL_U + 2 * CLR   # 16.1 x 9.6
Z_OUT = CLOCK_W / 2                              # outer end face
Z_PANEL = Z_OUT - PANEL_T                        # recess floor = 1.5 mm panel

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    v0 = body.volume
    tb = adsk.fusion.TemporaryBRepManager.get()
    x0, y0 = g(0, 0); xn, yn = g(1, 0); xu, yu = g(0, 1)
    DN = adsk.core.Vector3D.create(xn - x0, yn - y0, 0); DU = adsk.core.Vector3D.create(xu - x0, yu - y0, 0)
    def P(n, u, z):
        x, y = g(n, u); return adsk.core.Point3D.create(x / 10, y / 10, z / 10)
    def box(n0, n1, u0, u1, z0, z1):
        return tb.createBox(adsk.core.OrientedBoundingBox3D.create(P((n0 + n1) / 2, (u0 + u1) / 2, (z0 + z1) / 2), DN, DU, (n1 - n0) / 10, (u1 - u0) / 10, (z1 - z0) / 10))
    def rrect(wn, wu, r, z0, z1):   # rounded rectangle prism around the socket axis
        n0, n1, u0, u1 = SOCK_N - wn / 2, SOCK_N + wn / 2, SOCK_U - wu / 2, SOCK_U + wu / 2
        t = box(n0, n1, u0 + r, u1 - r, z0, z1)
        tb.booleanOperation(t, box(n0 + r, n1 - r, u0, u1, z0, z1), adsk.fusion.BooleanTypes.UnionBooleanType)
        for n in (n0 + r, n1 - r):
            for u in (u0 + r, u1 - r):
                tb.booleanOperation(t, tb.createCylinderOrCone(P(n, u, z0), r / 10, P(n, u, z1), r / 10), adsk.fusion.BooleanTypes.UnionBooleanType)
        return t
    # 1) fill the old inner pocket (only where it is empty now, minus the socket hole)
    fill = box(SOCK_N - POCKET_WN / 2, SOCK_N + POCKET_WN / 2, SOCK_U - POCKET_WU / 2, SOCK_U + POCKET_WU / 2, Z_IN, Z_PANEL)
    tb.booleanOperation(fill, tb.copy(body), adsk.fusion.BooleanTypes.DifferenceBooleanType)
    tb.booleanOperation(fill, rrect(SOCK_WN, SOCK_WU, 1.0, Z_IN - 2, Z_OUT + 2), adsk.fusion.BooleanTypes.DifferenceBooleanType)
    print('fill vol mm3', round(fill.volume * 1000, 1), 'expected about', round((POCKET_WN * POCKET_WU - SOCK_WN * SOCK_WU + 4 * (1 - math.pi / 4)) * PANEL_T, 1))
    # 2) outer recess for the flange
    rec = rrect(REC_N, REC_U, FL_R, Z_PANEL, Z_OUT + 2)
    for tool, op, nm in ((fill, adsk.fusion.FeatureOperations.JoinFeatureOperation, 'v13_usb_fill'),
                         (rec, adsk.fusion.FeatureOperations.CutFeatureOperation, 'v13_usb_recess')):
        bf = root.features.baseFeatures.add(); bf.startEdit()
        root.bRepBodies.add(tool, bf)
        bf.finishEdit(); bf.name = nm
        print(nm, 'base bodies', bf.bodies.count)
        tc = adsk.core.ObjectCollection.create(); tc.add(bf.bodies.item(0))
        ci = root.features.combineFeatures.createInput(body, tc); ci.operation = op
        root.features.combineFeatures.add(ci)
        body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    body.name = 'Stand_v13'
    print('bodies', root.bRepBodies.count, 'vol', round(v0, 4), '->', round(body.volume, 4))
    # rays: panel thickness at the hole edges, wall left above/below the recess on the end face
    for n, u, lab in ((SOCK_N, SOCK_U + SOCK_WU / 2 + 0.5, 'panel above hole'), (SOCK_N, SOCK_U - SOCK_WU / 2 - 0.5, 'panel below hole'),
                      (SOCK_N - SOCK_WN / 2 - 0.5, SOCK_U, 'panel front side'), (SOCK_N + SOCK_WN / 2 + 0.5, SOCK_U, 'panel rear side')):
        hits = adsk.core.ObjectCollection.create(); pts = adsk.core.ObjectCollection.create()
        root.findBRepUsingRay(P(n, u, Z_OUT + 5), adsk.core.Vector3D.create(0, 0, -1), adsk.fusion.BRepEntityTypes.BRepFaceEntityType, 0.001, False, pts)
        zs = [pts.item(i).z * 10 for i in range(pts.count)]
        print(lab, 'surfaces z', [round(z, 2) for z in zs[:3]])
    for u in (SOCK_U + REC_U / 2 + 0.01,):
        hits = adsk.core.ObjectCollection.create(); pts = adsk.core.ObjectCollection.create()
        x, y = g(0, u)
        root.findBRepUsingRay(P(SOCK_N + REC_N / 2, u, Z_OUT - 0.5), DN, adsk.fusion.BRepEntityTypes.BRepFaceEntityType, 0.001, False, pts)
        print('end-face material behind recess top-rear corner (mm to outside)', [round(P(SOCK_N + REC_N / 2, u, Z_OUT - 0.5).distanceTo(pts.item(i)) * 10, 2) for i in range(min(2, pts.count))])
