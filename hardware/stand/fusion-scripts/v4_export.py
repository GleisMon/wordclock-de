# print-oriented binary STL: table face down on the bed
import adsk.core, adsk.fusion, math, struct
OUT = 'wortuhr-stand_v4.stl'  # adjust to an absolute path
c, s = math.cos(math.radians(10)), math.sin(math.radians(10))

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = root.bRepBodies.itemByName('Stand_v4')
    tb = adsk.fusion.TemporaryBRepManager.get()
    t = tb.copy(body)
    m = adsk.core.Matrix3D.create()
    m.setWithArray([1, 0, 0, 0,      # print as it stands on the table: model Y-up -> Z-up
                    0, 0, -1, 0,
                    0, 1, 0, 0,
                    0, 0, 0, 1])
    tb.transform(t, m)
    bb = t.boundingBox
    mv = adsk.core.Matrix3D.create()
    mv.translation = adsk.core.Vector3D.create(-bb.minPoint.x, -bb.minPoint.y, -bb.minPoint.z)
    tb.transform(t, mv)
    mc = t.meshManager.createMeshCalculator()
    mc.setQuality(adsk.fusion.TriangleMeshQualityOptions.VeryHighQualityTriangleMesh)
    mesh = mc.calculate()
    co = mesh.nodeCoordinatesAsDouble
    idx = mesh.nodeIndices
    ntri = len(idx) // 3
    with open(OUT, 'wb') as f:
        f.write(b'WordClockStand v4'.ljust(80, b' '))
        f.write(struct.pack('<I', ntri))
        for i in range(ntri):
            p = [[co[3 * idx[3 * i + k] + j] * 10 for j in range(3)] for k in range(3)]
            u = [p[1][j] - p[0][j] for j in range(3)]; v = [p[2][j] - p[0][j] for j in range(3)]
            n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
            L = math.sqrt(sum(x * x for x in n)) or 1
            f.write(struct.pack('<12fH', *[x / L for x in n], *p[0], *p[1], *p[2], 0))
    bb = t.boundingBox
    print('tris', ntri, 'size mm', [round((getattr(bb.maxPoint, a) - getattr(bb.minPoint, a)) * 10, 2) for a in 'xyz'])
