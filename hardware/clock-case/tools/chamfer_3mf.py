# Chamfer all outer edges of the clock box and back plate (1 mm at print scale 1.2333) directly in the 3MF.
import re, sys, zipfile, numpy as np, manifold3d as m3
SRC = r'C:\Users\Master\Downloads\word_clock_GERMAN_60LED_per_meter.3mf'
DST = r'D:\___Claude\_Pub\Wortuhr\word_clock_GERMAN_60LED_per_meter_FASEN.3mf'
SCALE = 1.2333
C = 1.0 / SCALE            # chamfer in model units (1 mm printed)
C_SEAM = 0.5 / SCALE       # edges where box and plate meet (plate slab is only 2.47 mm)

def parse(xml, oid):
    m = re.search(r'(<object id="%s"[^>]*>\s*<mesh>)(.*?)(</mesh>)' % oid, xml, re.S)
    body = m.group(2)
    v = np.array([[float(a), float(b), float(c)] for a, b, c in re.findall(r'<vertex x="([^"]+)" y="([^"]+)" z="([^"]+)"', body)], dtype=np.float64)
    t = np.array([[int(a), int(b), int(c)] for a, b, c in re.findall(r'<triangle v1="(\d+)" v2="(\d+)" v3="(\d+)"', body)], dtype=np.uint32)
    return m, v, t

def prism(axis, lo, hi, corners):
    """Chamfered rectangle in the plane normal to `axis`, extruded far along it. corners = chamfer at (lo,lo),(hi,lo),(hi,hi),(lo,hi)."""
    i, j = [k for k in range(3) if k != axis]
    c0, c1, c2, c3 = corners
    poly = [(lo[i] + c0, lo[j]), (hi[i] - c1, lo[j]), (hi[i], lo[j] + c1), (hi[i], hi[j] - c2),
            (hi[i] - c2, hi[j]), (lo[i] + c3, hi[j]), (lo[i], hi[j] - c3), (lo[i], lo[j] + c0)]
    pts = []
    for w in (lo[axis] - 5, hi[axis] + 5):
        for a, b in poly:
            p = [0, 0, 0]; p[axis] = w; p[i] = a; p[j] = b; pts.append(p)
    return m3.Manifold.hull_points(np.array(pts, dtype=np.float64))

def chamfer(xml, oid, name, zlo, zhi, c_zlo, c_zhi):
    """Outer slab spans zlo..zhi; edges at zlo get c_zlo, edges at zhi get c_zhi, the four corner edges C."""
    m, v, t = parse(xml, oid)
    man = m3.Manifold(m3.Mesh(vert_properties=v.astype(np.float32), tri_verts=t))
    assert man.status() == m3.Error.NoError, man.status()
    lo, hi = v.min(0), v.max(0)
    lo2, hi2 = lo.copy(), hi.copy(); lo2[2], hi2[2] = zlo, zhi
    keep = prism(2, lo2, hi2, (C, C, C, C)) ^ prism(0, lo2, hi2, (c_zlo, c_zlo, c_zhi, c_zhi)) ^ prism(1, lo2, hi2, (c_zlo, c_zhi, c_zhi, c_zlo))
    if hi[2] > zhi + 1e-6:   # ribs / bosses standing on the slab stay untouched
        ins = max(C, c_zhi) + 0.5
        keep += m3.Manifold.cube([hi[0] - lo[0] - 2 * ins, hi[1] - lo[1] - 2 * ins, hi[2] - zhi + 1.0]).translate([lo[0] + ins, lo[1] + ins, zhi - 0.5])
    out = man ^ keep
    mesh = out.to_mesh()
    nv = np.asarray(mesh.vert_properties)[:, :3]; nt = np.asarray(mesh.tri_verts)
    print(name, 'size', np.round((hi - lo) * SCALE, 2), 'verts %d->%d tris %d->%d' % (len(v), len(nv), len(t), len(nt)),
          'vol %.1f -> %.1f mm3 (removed %.1f)' % (man.volume() * SCALE**3, out.volume() * SCALE**3, (man.volume() - out.volume()) * SCALE**3), 'genus', man.genus(), out.genus())
    NL = chr(10)
    body = NL + '    <vertices>' + NL + ''.join('     <vertex x="%.7g" y="%.7g" z="%.7g"/>' % tuple(p) + NL for p in nv) + '    </vertices>' + NL \
         + '    <triangles>' + NL + ''.join('     <triangle v1="%d" v2="%d" v3="%d"/>' % tuple(q) + NL for q in nt) + '    </triangles>' + NL + '   '
    return xml[:m.start(2)] + body + xml[m.end(2):], (nv, nt)

zin = zipfile.ZipFile(SRC)
zout = zipfile.ZipFile(DST, 'w', zipfile.ZIP_DEFLATED)
meshes = {}
for info in zin.infolist():
    data = zin.read(info.filename)
    if info.filename == '3D/Objects/object_1.model':
        x, meshes['box'] = chamfer(data.decode('utf-8'), '1', 'box', -6.0, 6.0, C, C_SEAM); data = x.encode('utf-8')
    elif info.filename == '3D/Objects/object_2.model':
        x, meshes['plate'] = chamfer(data.decode('utf-8'), '5', 'plate', -1.5, 0.5, C, C_SEAM); data = x.encode('utf-8')
    zout.writestr(info.filename, data)
zout.close()
import pickle; pickle.dump(meshes, open('fasen_meshes.pkl', 'wb'))
print('written', DST)
