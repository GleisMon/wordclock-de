# print-oriented binary STL (sloped rear wall with the OLED window down on the bed) + STEP
import adsk.core, adsk.fusion, math, struct, os
exec(open(r'D:\___Claude\_Pub\Wortuhr\entwicklung\fusion_v10\edges_dump.py', encoding='utf-8').read().split('\ndef convex_info')[0])
OUT_USER = r'D:\___Claude\_Pub\Wortuhr\wortuhr-staender_druck_v13_oled.stl'
OUT_REPO = r'D:\___Claude\_Pub\Wortuhr\wordclock-de\hardware\stand\wortuhr-stand_v13'

def run(_context: str):
    app = adsk.core.Application.get()
    des = adsk.fusion.Design.cast(app.activeProduct)
    root = des.rootComponent
    body = root.bRepBodies.itemByName('Stand_v13')
    ax, ay = g(32.5, 16.0); bx, by = g(0.0, 52.0)
    dx, dy = bx - ax, by - ay; L = math.hypot(dx, dy)
    Nx, Ny = dy / L, -dx / L
    cx, cy = g(10.0, 20.0)
    if (ax - cx) * Nx + (ay - cy) * Ny < 0: Nx, Ny = -Nx, -Ny   # outward
    tb = adsk.fusion.TemporaryBRepManager.get()
    t = tb.copy(body)
    m = adsk.core.Matrix3D.create()
    m.setWithArray([0, 0, 1, 0,  -Ny, Nx, 0, 0,  -Nx, -Ny, 0, 0,  0, 0, 0, 1])
    tb.transform(t, m)
    bb = t.boundingBox
    mv = adsk.core.Matrix3D.create()
    mv.translation = adsk.core.Vector3D.create(-bb.minPoint.x, -bb.minPoint.y, -bb.minPoint.z)
    tb.transform(t, mv)
    def write(tbody, outs):
        mc = tbody.meshManager.createMeshCalculator()
        mc.setQuality(adsk.fusion.TriangleMeshQualityOptions.VeryHighQualityTriangleMesh)
        mesh = mc.calculate()
        co = mesh.nodeCoordinatesAsDouble; idx = mesh.nodeIndices; ntri = len(idx) // 3
        for out in outs:
            with open(out, 'wb') as f:
                f.write(b'Wortuhr stand v13'.ljust(80, b' '))
                f.write(struct.pack('<I', ntri))
                for i in range(ntri):
                    p = [[co[3 * idx[3 * i + k] + j] * 10 for j in range(3)] for k in range(3)]
                    u = [p[1][j] - p[0][j] for j in range(3)]; v = [p[2][j] - p[0][j] for j in range(3)]
                    n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
                    LL = math.sqrt(sum(x * x for x in n)) or 1
                    f.write(struct.pack('<12fH', *[x / LL for x in n], *p[0], *p[1], *p[2], 0))
        return ntri
    ntri = write(t, (OUT_USER, OUT_REPO + '.stl'))
    # test slice: the 22 mm at the USB-C end (print x = model z, already shifted to start at 0)
    bb = t.boundingBox
    L = bb.maxPoint.x
    box = tb.createBox(adsk.core.OrientedBoundingBox3D.create(adsk.core.Point3D.create(L - 1.1, 3, 3), adsk.core.Vector3D.create(1, 0, 0), adsk.core.Vector3D.create(0, 1, 0), 2.2, 20, 20))
    sl = tb.copy(t)
    tb.booleanOperation(sl, box, adsk.fusion.BooleanTypes.IntersectionBooleanType)
    mv2 = adsk.core.Matrix3D.create(); mv2.translation = adsk.core.Vector3D.create(-sl.boundingBox.minPoint.x, 0, 0)
    tb.transform(sl, mv2)
    write(sl, (r'D:\___Claude\_Pub\Wortuhr\wortuhr-staender_v13_PROBE_zuerst_drucken.stl', OUT_REPO + '_testslice.stl'))
    print('slice size', [round((getattr(sl.boundingBox.maxPoint, a) - getattr(sl.boundingBox.minPoint, a)) * 10, 2) for a in 'xyz'], 'vol', round(sl.volume, 2))
    bb = t.boundingBox
    zmin_area = sum(f.area for f in t.faces if abs(f.boundingBox.maxPoint.z) < 1e-5) * 100
    em = des.exportManager
    for o in root.occurrences: o.isLightBulbOn = False
    em.execute(em.createSTEPExportOptions(OUT_REPO + '.step', root))
    print('tris', ntri, 'size mm', [round((getattr(bb.maxPoint, a) - getattr(bb.minPoint, a)) * 10, 2) for a in 'xyz'], 'bed area mm2', round(zmin_area), 'vol cm3', round(body.volume, 2))
