# v11 = v10 with the lower holes for the new screws: thread 4.7, head 7.8 x 2.7, total length 42.6 mm
import adsk.core, adsk.fusion, math, os
exec(open(r'D:\___Claude\_Pub\Wortuhr\entwicklung\fusion_v10\edges_dump.py', encoding='utf-8').read().split('\ndef convex_info')[0])
D_THRU11, D_CB11, CLAMP11 = 5.2, 8.4, 29.5   # head seat 3.0 mm below the rear face (n = 32.5)

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    v0 = body.volume
    tb = adsk.fusion.TemporaryBRepManager.get()
    def P(n, u, z):
        x, y = g(n, u); return adsk.core.Point3D.create(x / 10, y / 10, z / 10)
    tool = None
    for z in (HOLE_Z, -HOLE_Z):
        for n0, n1, d in ((-1.0, 40.0, D_THRU11), (CLAMP11, 40.0, D_CB11)):
            cyl = tb.createCylinderOrCone(P(n0, HOLE_U, z), d / 20, P(n1, HOLE_U, z), d / 20)
            if tool is None: tool = cyl
            else: tb.booleanOperation(tool, cyl, adsk.fusion.BooleanTypes.UnionBooleanType)
    bf = root.features.baseFeatures.add(); bf.startEdit()
    root.bRepBodies.add(tool, bf)
    bf.finishEdit(); bf.name = 'v11_screw_4.7'
    tc = adsk.core.ObjectCollection.create(); tc.add(bf.bodies.item(0))
    ci = root.features.combineFeatures.createInput(body, tc); ci.operation = adsk.fusion.FeatureOperations.CutFeatureOperation
    root.features.combineFeatures.add(ci)
    body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    body.name = 'Stand_v11'
    print('HOLE_U', HOLE_U, 'HOLE_Z', HOLE_Z, 'vol', round(v0, 3), '->', round(body.volume, 3), 'bodies', root.bRepBodies.count)
    # wall checks by rays: from hole axis at counterbore depth outwards
    for z in (HOLE_Z, -HOLE_Z):
        for nn in (15.0, 31.0):
            c = P(nn, HOLE_U, z); res = []
            for name, dn, du, dz in (('up', 0, 1, 0), ('down', 0, -1, 0), ('end', 0, 0, 1 if z > 0 else -1), ('in', 0, 0, -1 if z > 0 else 1)):
                x0, y0 = g(0, 0); x1, y1 = g(dn, du)
                d = adsk.core.Vector3D.create(x1 - x0, y1 - y0, dz); d.normalize()
                hits = adsk.core.ObjectCollection.create()
                root.findBRepUsingRay(c, d, adsk.fusion.BRepEntityTypes.BRepFaceEntityType, 0.001, False, hits)
                ds = sorted(c.distanceTo(h) * 10 for h in hits)
                res.append('%s %s' % (name, [round(x, 2) for x in ds[:3]]))
            print('z', round(z, 1), 'n', nn, ' | '.join(res))
