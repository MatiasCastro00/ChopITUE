import struct, zlib, pathlib
p = pathlib.Path(__file__).resolve().parents[1] / 'SourceArt/Mandrake/palmera_dos_materiales.fbx'
b = p.read_bytes()
def prop(pos):
    t = chr(b[pos]); pos += 1
    if t in 'YCLFDI':
        fmt = {'Y':'h','C':'?','L':'q','F':'f','D':'d','I':'i'}[t]
        return pos+struct.calcsize(fmt), struct.unpack_from('<'+fmt,b,pos)[0]
    if t in 'SR':
        size=struct.unpack_from('<I',b,pos)[0]; pos+=4
        return pos+size,b[pos:pos+size]
    if t in 'fdilb':
        count,enc,size=struct.unpack_from('<III',b,pos);pos+=12
        raw=b[pos:pos+size]
        if enc: raw=zlib.decompress(raw)
        fmt={'f':'f','d':'d','i':'i','l':'q','b':'?'}[t]
        return pos+size,struct.unpack('<'+str(count)+fmt,raw)
    raise ValueError(('Unknown property',pos-1,t))
def node(pos,depth=0):
    start=pos
    end,count,psize,nsize=struct.unpack_from('<IIIB',b,pos);pos+=13
    if not end:return pos,None
    name=b[pos:pos+nsize].decode();pos+=nsize
    props=[];pstart=pos
    for i in range(count):
        pos,v=prop(pos);props.append(v)
    if pos-pstart!=psize: print('PROP_SIZE_MISMATCH',start,name,psize,pos-pstart)
    children=[]
    while pos<end:
        if b[pos:pos+13]==bytes(13):pos+=13;break
        pos,n=node(pos,depth+1);children.append(n)
    if pos!=end: print('END_MISMATCH',start,name,end,pos)
    return pos,dict(name=name,props=props,children=children)
pos=27;nodes=[]
try:
    while pos<len(b):
        start=pos;pos,n=node(pos)
        if not n:break
        nodes.append(n);print('NODE',start,pos,n['name'])
except Exception as e:
    print('PARSE_ERROR',pos,e);raise
def walk(nodes):
    for n in nodes:
        if not n: continue
        if n['name'] in ('Geometry','Material','Model'):print(n['name'],n['props'][:3])
        if n['name']=='Vertices':
            v=n['props'][0];print('BOUNDS',[(min(v[i::3]),max(v[i::3])) for i in range(3)])
        walk(n['children'])
walk(nodes)

def value(v):
    if isinstance(v,bytes):
        s=v.decode('utf-8',errors='replace')
        if '\x00\x01' in s:
            name,kind=s.split('\x00\x01',1);s=kind+'::'+name
        return '"'+s.replace('\\','/').replace('"','\\"')+'"'
    if isinstance(v,bool):return str(int(v))
    return str(v)
def ascii_node(n,depth=0):
    indent='\t'*depth
    if n['name']=='FileId':return ''
    if len(n['props'])==1 and isinstance(n['props'][0],tuple):
        a=n['props'][0]
        return indent+n['name']+': *'+str(len(a))+' {\n'+indent+'\ta: '+','.join(value(v) for v in a)+'\n'+indent+'}\n'
    s=indent+n['name']+': '+', '.join(value(v) for v in n['props'])
    if n['children']:
        s+=' {\n'+''.join(ascii_node(c,depth+1) for c in n['children'] if c)+indent+'}'
    return s+'\n'
out=p.with_name('Mandrake_Recovered.fbx')
out.write_text('; FBX 7.4.0 project file\n'+''.join(ascii_node(n) for n in nodes),encoding='utf-8')
print('RECOVERED',out)
