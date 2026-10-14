# v12 = v11 with slightly rounded corners of the USB-C cutout (user 14.10 after the test print): R1.0
import adsk.core, adsk.fusion, math, os
exec(open(r'D:\___Claude\_Pub\Wortuhr\entwicklung\fusion_v10\edges_dump.py', encoding='utf-8').read().split('\ndef convex_info')[0])
R = 0.10  # cm

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    v0 = body.volume
    edges = adsk.core.ObjectCollection.create()
    for e in body.edges:
        if e.geometry.objectType != adsk.core.Line3D.classType(): continue
        a, b = e.startVertex.geometry, e.endVertex.geometry
        if abs(a.x - b.x) > 1e-5 or abs(a.y - b.y) > 1e-5: continue           # parallel to the stand axis
        if min(a.z, b.z) * 10 < 113.5 or abs(a.z - b.z) * 10 > 2.0: continue   # through the 1.5 mm end panel at +z
        n, u = nu(a.x * 10, a.y * 10)
        if 7.5 < n < 22.5 and 15.0 < u < 22.0:
            edges.add(e); print('corner n %.2f u %.2f len %.2f' % (n, u, e.length * 10))
    assert edges.count == 4, edges.count
    fi = root.features.filletFeatures.createInput()
    fi.addConstantRadiusEdgeSet(edges, adsk.core.ValueInput.createByReal(R), False)
    root.features.filletFeatures.add(fi).name = 'v12_usb_round_R1'
    body.name = 'Stand_v12'
    print('vol', round(v0, 4), '->', round(body.volume, 4))
