"""Author editable Mandrake mesh materials and CPU Niagara systems in Unreal."""
import unreal
import sys

ROOT = '/Game/ChopIt/Items/Mandrake'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
FX = unreal.FXConverterUtilitiesLibrary
MEL = unreal.MaterialEditingLibrary

def material(name, translucent=False):
    old = unreal.load_asset(ROOT+'/'+name)
    if old:
        return old, False
    m = TOOLS.create_asset(name, ROOT, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property('two_sided', True)
    if translucent:
        m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    return m, True

def expr(m, kind, **props):
    n = MEL.create_material_expression(m, getattr(unreal, 'MaterialExpression'+kind))
    for k,v in props.items(): n.set_editor_property(k,v)
    return n

def scalar(m, x): return expr(m, 'Constant', r=x)
def vector(m, xyz): return expr(m, 'Constant3Vector', constant=unreal.LinearColor(*xyz,1))
def link(a, b, pin, output=''): MEL.connect_material_expressions(a,output,b,pin)
def output(n, prop, channel=''): MEL.connect_material_property(n,channel,getattr(unreal.MaterialProperty,'MP_'+prop))
def mul(m,a,b):
    n=expr(m,'Multiply');link(a,n,'A');link(b,n,'B');return n
def custom(m, code, inputs, float3=False):
    n=expr(m,'Custom', code=code, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3 if float3 else unreal.CustomMaterialOutputType.CMOT_FLOAT1)
    pins=[]
    for k in inputs:
        pin=unreal.CustomInput();pin.set_editor_property('input_name',k);pins.append(pin)
    n.set_editor_property('inputs',pins)
    for k,v in inputs.items():link(v,n,k)
    return n
def save(m):
    MEL.recompile_material(m)
    if not unreal.EditorAssetLibrary.save_loaded_asset(m,only_if_is_dirty=False): raise RuntimeError(m.get_name())

body,new=material('M_Mandrake_Body')
if new:
    tex=expr(body,'TextureSample',texture=unreal.load_asset(ROOT+'/T_Mandrake_Albedo'))
    output(tex,'BASE_COLOR','RGB');output(scalar(body,.82),'ROUGHNESS')
    output(mul(body,tex,scalar(body,.16)),'EMISSIVE_COLOR');save(body)

tears,new=material('M_Mandrake_TearsFlow',True)
if new:
    uv=expr(tears,'TextureCoordinate')
    pan=expr(tears,'Panner',speed_x=.08,speed_y=-.8);link(uv,pan,'Coordinate')
    time=expr(tears,'Time');pos=expr(tears,'WorldPosition')
    flow=custom(tears,'float streak=pow(saturate(0.5+0.5*sin(UV.x*110+sin(UV.y*17))),5); float fall=pow(saturate(0.5+0.5*sin(P.z*0.65+T*20+sin(UV.x*29))),9); return saturate(streak*0.35+fall*0.8);',{'UV':pan,'P':pos,'T':time})
    col=expr(tears,'LinearInterpolate');link(vector(tears,(.05,.3,.65)),col,'A');link(vector(tears,(.7,.95,1)),col,'B');link(flow,col,'Alpha')
    output(col,'BASE_COLOR');output(mul(tears,col,scalar(tears,.6)),'EMISSIVE_COLOR')
    output(scalar(tears,.78),'OPACITY');output(scalar(tears,.08),'ROUGHNESS');output(scalar(tears,.9),'SPECULAR')
    output(scalar(tears,1.025),'REFRACTION')
    normal=custom(tears,'return normalize(float3(sin(UV.x*50+UV.y*15)*.17,cos(UV.y*42)*.13,1));',{'UV':pan},True)
    output(normal,'NORMAL');save(tears)

wave,new=material('M_Mandrake_SonicRefraction',True)
if new:
    wave.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    fres=expr(wave,'Fresnel',exponent=5.0,base_reflect_fraction=.0)
    pc=expr(wave,'ParticleColor')
    alpha=mul(wave,fres,pc);MEL.connect_material_expressions(pc,'A',alpha,'B')
    output(mul(wave,alpha,scalar(wave,.22)),'OPACITY')
    output(mul(wave,vector(wave,(.22,.8,1.4)),fres),'EMISSIVE_COLOR')
    refr=expr(wave,'Add');link(scalar(wave,1),refr,'A');link(mul(wave,alpha,scalar(wave,.035)),refr,'B');output(refr,'REFRACTION')
    save(wave)

def sprite_material(name,star):
    m,new=material(name,True)
    if new:
        m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        uv=expr(m,'TextureCoordinate');pc=expr(m,'ParticleColor')
        code='float2 p=UV*2-1; float a=atan2(p.y,p.x); float r=length(p); return saturate((0.62+0.22*cos(a*5)-r)*20);' if star else 'float2 p=(UV-.5)*2; return pow(saturate(1-dot(p,p)),2);'
        mask=custom(m,code,{'UV':uv});a=mul(m,mask,pc);MEL.connect_material_expressions(pc,'A',a,'B')
        output(a,'OPACITY');output(mul(m,pc,scalar(m,2)),'EMISSIVE_COLOR');save(m)
    return m
star=sprite_material('M_Mandrake_ConfusionStar',True)
drop=sprite_material('M_Mandrake_Droplet',False)
for refractive in [tears,wave]:
    refractive.set_editor_property('refraction_method',unreal.RefractionMode.RM_INDEX_OF_REFRACTION)
    save(refractive)

mesh=unreal.load_asset(ROOT+'/SM_Mandrake')
for i,slot in enumerate(mesh.get_editor_property('static_materials')):
    mesh.set_material(i,tears if str(slot.material_slot_name)=='Lagrimas' else body)
unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)

def script(path,version=None):
    data=FX.create_asset_data(path)
    return unreal.CreateScriptContextArgs(data,version) if version else unreal.CreateScriptContextArgs(data)
def rand(lo,hi):
    s=FX.create_script_context(script('/Niagara/DynamicInputs/UniformRange/V2/RandomRangeFloat.RandomRangeFloat'))
    setp(s,'Minimum',FX.create_script_input_float(lo));setp(s,'Maximum',FX.create_script_input_float(hi))
    return FX.create_script_input_dynamic(s,unreal.NiagaraScriptInputType.FLOAT)
def color(c):
    s=FX.create_script_context(script('/Niagara/DynamicInputs/LinearColor/MakeLinearColorFromVectorAndFloat.MakeLinearColorFromVectorAndFloat'))
    setp(s,'Vector (RGB)',FX.create_script_input_vector(unreal.Vector(*c[:3])))
    setp(s,'Float (Alpha)',FX.create_script_input_float(c[3]))
    return FX.create_script_input_dynamic(s,unreal.NiagaraScriptInputType.LINEAR_COLOR)
def module(e,name,path,category,version=None):return e.find_or_add_module_script(name,script(path,version),category)
def setp(context,name,value,*condition):
    if not context.set_parameter(name,value,*condition):raise RuntimeError("Niagara input not found: "+name)
    return True

ES=unreal.ScriptExecutionCategory
def emitter(ctx,name,rate,radius,life,size,c,mat,velocity=(0,0,0),gravity=False,burst=False,shell=False):
    e=ctx.add_empty_emitter(name);e.set_local_space(not gravity);e.set_sim_target(unreal.NiagaraSimTarget.CPU_SIM)
    state=module(e,'EmitterState','/Niagara/Modules/Emitter/EmitterState.EmitterState',ES.EMITTER_UPDATE,[1,0])
    setp(state,'Life Cycle Mode',FX.create_script_input_enum('/Niagara/Enums/ENiagaraEmitterLifeCycleMode.ENiagaraEmitterLifeCycleMode','Self'))
    setp(state,'Loop Behavior',FX.create_script_input_enum('/Niagara/Enums/ENiagara_EmitterStateOptions.ENiagara_EmitterStateOptions','Once' if burst else 'Infinite'))
    setp(state,'Loop Duration',FX.create_script_input_float(life if burst else 1.0))
    if burst:
        spawn=module(e,'SpawnBurst','/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous',ES.EMITTER_UPDATE,[1,1])
        setp(spawn,'Spawn Count',FX.create_script_input_int(int(rate)))
    else:
        spawn=module(e,'SpawnRate','/Niagara/Modules/Emitter/SpawnRate.SpawnRate',ES.EMITTER_UPDATE)
        assert setp(spawn,'SpawnRate',FX.create_script_input_float(rate))
    init=module(e,'InitializeParticle','/Niagara/Modules/Spawn/Initialization/V2/InitializeParticle.InitializeParticle',ES.PARTICLE_SPAWN,[1,0])
    setp(init,'Lifetime',FX.create_script_input_float(life) if shell else rand(life*.7,life*1.2))
    setp(init,'Color',color(c))
    if shell:
        setp(init,'Mesh Scale Mode',FX.create_script_input_enum('/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode','Non-Uniform'))
        setp(init,'Mesh Scale',FX.create_script_input_vector(unreal.Vector(1,1,1)))
        scale=module(e,'ExpandingWave','/Niagara/Modules/Update/Size/ScaleMeshSize.ScaleMeshSize',ES.PARTICLE_UPDATE)
        v=FX.create_script_context(script('/Niagara/DynamicInputs/Multiply/Multiply_VectorByFloat.Multiply_VectorByFloat'))
        setp(v,'Vector',FX.create_script_input_vector(unreal.Vector(7,7,4)))
        setp(v,'Float',FX.create_script_input_linked_parameter('Particles.NormalizedAge',unreal.NiagaraScriptInputType.FLOAT))
        setp(scale,'Scale Factor',FX.create_script_input_dynamic(v,unreal.NiagaraScriptInputType.VEC3))
    else:
        setp(init,'Sprite Size Mode',FX.create_script_input_enum('/Niagara/Enums/ENiagara_SizeScaleMode.ENiagara_SizeScaleMode','Non-Uniform'))
        setp(init,'Sprite Size',FX.create_script_input_vec2(unreal.Vector2D(size,size)),True,True)
    if radius:
        loc=module(e,'RingLocation','/Niagara/Modules/Spawn/Location/V2/ShapeLocation.ShapeLocation',ES.PARTICLE_SPAWN)
        setp(loc,'Shape Primitive',FX.create_script_input_enum('/Niagara/Enums/Location/ENiagara_LocationShapes.ENiagara_LocationShapes','Ring / Disc'))
        setp(loc,'Ring Radius',FX.create_script_input_float(radius))
    if any(velocity):
        vel=module(e,'Lift','/Niagara/Modules/Spawn/Velocity/AddVelocity.AddVelocity',ES.PARTICLE_SPAWN)
        setp(vel,'Velocity Mode',FX.create_script_input_enum('/Niagara/Enums/Utility/ENiagara_VelocityMode.ENiagara_VelocityMode','Linear'))
        scatter=FX.create_script_context(script('/Niagara/DynamicInputs/UniformRange/V2/RandomRangeVector.RandomRangeVector'))
        setp(scatter,'Minimum',FX.create_script_input_vector(unreal.Vector(-45,-45,velocity[2]*.65)))
        setp(scatter,'Maximum',FX.create_script_input_vector(unreal.Vector(45,45,velocity[2]*1.3)))
        setp(vel,'Velocity',FX.create_script_input_dynamic(scatter,unreal.NiagaraScriptInputType.VEC3))
    if gravity:
        g=module(e,'Gravity','/Niagara/Modules/Update/Forces/GravityForce.GravityForce',ES.PARTICLE_UPDATE)
        setp(g,'Gravity',FX.create_script_input_vector(unreal.Vector(0,0,-380)))
    module(e,'Solve','/Niagara/Modules/Solvers/SolveForcesAndVelocity.SolveForcesAndVelocity',ES.PARTICLE_UPDATE)
    module(e,'ParticleState','/Niagara/Modules/Update/Lifetime/ParticleState.ParticleState',ES.PARTICLE_UPDATE,[1,1])
    fade=module(e,'Fade','/Niagara/Modules/Update/Color/ScaleColor.ScaleColor',ES.PARTICLE_UPDATE)
    inv=FX.create_script_context(script('/Niagara/DynamicInputs/Math/OneMinusFloat.OneMinusFloat'))
    setp(inv,'Float',FX.create_script_input_linked_parameter('Particles.NormalizedAge',unreal.NiagaraScriptInputType.FLOAT))
    setp(fade,'Scale Alpha',FX.create_script_input_dynamic(inv,unreal.NiagaraScriptInputType.FLOAT),True,True)
    if shell:
        renderer=unreal.NiagaraMeshRendererProperties()
        entry=unreal.NiagaraMeshRendererMeshProperties();entry.set_editor_property('mesh',unreal.load_asset('/Engine/BasicShapes/Sphere.Sphere'))
        renderer.set_editor_property('meshes',[entry]);renderer.set_editor_property('bOverrideMaterials',True)
        override=unreal.NiagaraMeshMaterialOverride();override.set_editor_property('explicit_mat',mat)
        renderer.set_editor_property('OverrideMaterials',[override])
    else:
        renderer=unreal.NiagaraSpriteRendererProperties();renderer.set_editor_property('material',mat)
    e.add_renderer(name+'Renderer',renderer);e.finalize()
    return renderer

def system(name,build):
    if '--rebuild-niagara' in sys.argv and unreal.EditorAssetLibrary.does_asset_exist(ROOT+'/'+name):
        unreal.EditorAssetLibrary.delete_asset(ROOT+'/'+name)
    existing=unreal.load_asset(ROOT+'/'+name)
    if existing:return existing
    asset=TOOLS.create_asset(name,ROOT,unreal.NiagaraSystem,unreal.NiagaraSystemFactoryNew())
    ctx=FX.create_system_conversion_context(asset)
    build(ctx)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset,only_if_is_dirty=False):raise RuntimeError(name)
    ctx.cleanup()
    unreal.log('MANDRAKE_VFX_READY '+name)
    return asset

for m in [body,tears]:
    MEL.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_STATIC_LIGHTING)
MEL.set_material_usage(wave,unreal.MaterialUsage.MATUSAGE_NIAGARA_MESH_PARTICLES)
for m in [star,drop]:MEL.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_NIAGARA_SPRITES)
for m in [body,tears,wave,star,drop]:save(m)

system('NS_Mandrake_SonicWaves',lambda c: emitter(c,'RefractiveSoundShells',2,0,.8,1,(.3,.8,1,1),wave,shell=True))
system('NS_Mandrake_TearSplash',lambda c: emitter(c,'WaterDroplets',32,5,.6,3,(.4,.75,1,.9),drop,(10,8,100),gravity=True))
system('NS_Mandrake_Confusion',lambda c: emitter(c,'ConfusedStars',8,24,.85,11,(1,.55,.12,.95),star))
system('NS_Mandrake_Emerge',lambda c: emitter(c,'ForestMotes',36,28,.8,5,(.3,.8,.12,.8),drop,(15,10,65),gravity=True,burst=True))

item=unreal.load_asset('/Game/ChopIt/Items/DA_Item_MandrakeStick')
item.set_editor_property('description','Al matar a un enemigo, 10% de probabilidad de invocar una mandrágora durante 6 s. Su grito causa 8 de daño por segundo y ralentiza un 35% a los enemigos cercanos. La confusión dura hasta 1 s después del último impacto. Los stacks aumentan la probabilidad con rendimiento decreciente.')
if not unreal.EditorAssetLibrary.save_loaded_asset(item,only_if_is_dirty=False):raise RuntimeError('Could not save Mandrake item description')
