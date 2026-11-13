"""Editor authoring specification. Generated assets run entirely as standard Blueprint graphs."""
import json
from pathlib import Path

ROOT='/Game/Blueprints/Agents/EnemyINProgress/BlueprintOnly'
def cls(name): return ROOT+'/'+name+'.'+name+'_C'
CHAR=cls('BP_Enemy'); HEALTH=cls('AC_EnemyHealth_BP'); COMBAT=cls('AC_EnemyCombat_BP'); BRIDGE=cls('BP_PlayerBridge'); SPAWNER=cls('BP_EnemySpawner')
M='/Script/Engine.KismetMathLibrary'; K='/Script/Engine.KismetSystemLibrary'; GS='/Script/Engine.GameplayStatics'; AR='/Script/Engine.KismetArrayLibrary'
ACT='/Script/Engine.Actor'; PAWN='/Script/Engine.Pawn'; CH='/Script/Engine.Character'; CTRL='/Script/Engine.Controller'; MOVE='/Script/Engine.CharacterMovementComponent'; ANIM='/Script/Engine.AnimInstance'; SK='/Script/Engine.SkeletalMeshComponent'
NAV='/Script/NavigationSystem.NavigationSystemV1'; AI='/Script/AIModule.AIBlueprintHelperLibrary'; AIC='/Script/AIModule.AIController'; PC='/Script/Engine.PlayerController'
WIDGET='/Script/UMG.WidgetComponent'; SM='/Script/Engine.StaticMeshComponent'; PRIM='/Script/Engine.PrimitiveComponent'; SCENE='/Script/Engine.SceneComponent'
PLAYER='/Game/Blueprints/Agents/Player-Agent/BP_FirstPersonCharacter.BP_FirstPersonCharacter_C'
HB='/Game/Blueprints/CombatSystem/Health/WBP_Healthbar.WBP_Healthbar_C'
NURSE='/Game/Blueprints/Agents/Enemy-Agent/Models/Model_Nurse/Ch11_nonPBR.Ch11_nonPBR'
BASE='/Game/Blueprints/Agents/EnemyINProgress'
class R(str): pass
def val(x):
    if x is None: return ''
    if isinstance(x,bool): return 'true' if x else 'false'
    return str(x)
def var(name,type='double',default=0,**kw): return dict(name=name,type=type,default=val(default),**kw)
def arg(name,type='double',**kw): return dict(name=name,type=type,**kw)
def ob(name,c,default=None,**kw): return var(name,'object',default,**{'class':c},**kw)
class Graph:
    def __init__(this,name,inputs=(),outputs=(),pure=False):
        this.d=dict(name=name,inputs=list(inputs),outputs=list(outputs),pure=pure,nodes=[],links=[])
        this.tail=[] if name=='EventGraph' or pure else [R('entry.then')]
        this.n=0
    def node(this,kind,**kw):
        this.n+=1; id='n'+str(this.n); this.d['nodes'].append(dict(id=id,kind=kind,**kw)); return id
    def link(this,a,b): this.d['links'].append([str(a),str(b)])
    def feed(this,id,**inputs):
        defaults={}
        for k,v in inputs.items():
            if isinstance(v,R): this.link(v,id+'.'+k)
            else: defaults[k]=val(v)
        for n in this.d['nodes']:
            if n['id']==id: n.setdefault('defaults',{}).update(defaults)
    def ref(this,id,p='ReturnValue'): return R(id+'.'+p)
    def inp(this,name): return R('entry.'+name)
    def self(this): return this.ref(this.node('self'),'self')
    def get(this,name,obj=None,owner=None):
        if obj is None and owner in (ACT,PAWN,CH): owner=None
        n=this.node('get',variable=name,**({'owner':owner} if owner else {}))
        if obj is not None: this.feed(n,self=obj)
        return this.ref(n,name)
    def call(this,f,owner=None,out='ReturnValue',**inputs):
        n=this.node('call',function=f,**({'owner':owner} if owner else {})); this.feed(n,**inputs); return this.ref(n,out)
    def execnode(this,n,inpin='execute',outpin='then'):
        for t in this.tail: this.link(t,n+'.'+inpin)
        this.tail=[this.ref(n,outpin)]
        return n
    def do(this,f,owner=None,force=False,**inputs):
        n=this.node('call',function=f,force_impure=force,**({'owner':owner} if owner else {})); this.feed(n,**inputs); this.execnode(n); return n
    def set(this,name,v,obj=None,owner=None):
        if obj is None and owner in (ACT,PAWN,CH): owner=None
        n=this.node('set',variable=name,**({'owner':owner} if owner else {})); this.feed(n,**{name:v})
        if obj is not None: this.feed(n,self=obj)
        this.execnode(n); return this.ref(n,'Output_Get')
    def branch(this,cond,yes,no=None):
        n=this.node('branch'); this.feed(n,Condition=cond); this.execnode(n)
        this.tail=[this.ref(n,'then')]; yes(); y=this.tail[:]
        this.tail=[this.ref(n,'else')]
        if no: no()
        this.tail=y+this.tail
    def end(this,**outs):
        if this.d['outputs'] or this.d.get('return'):
            for t in this.tail: this.link(t,'result.execute')
            this.feed_result(outs)
        this.tail=[]
    def feed_result(this,outs):
        for k,v in outs.items():
            if isinstance(v,R): this.link(v,'result.'+k)
            else: this.d.setdefault('defaults',{})[k]=val(v)
    def cast(this,obj,c):
        n=this.node('cast',**{'class':c},pure=True); this.feed(n,Object=obj); return this.ref(n,'As')
    def event(this,f,owner):
        n=this.node('event',function=f,owner=owner); this.tail=[this.ref(n,'then')]; return n
    def array(this,items):
        n=this.node('array',count=max(1,len(items)))
        for i,x in enumerate(items): this.feed(n,**{'['+str(i)+']':x})
        return this.ref(n,'Array')
    def foreach(this,items,body):
        n=this.node('macro',macro='ForEachLoop'); this.feed(n,Array=items); this.execnode(n,'Exec','LoopBody')
        body(this.ref(n,'Array Element'),this.ref(n,'Array Index'))
        this.tail=[this.ref(n,'Completed')]
    def spawn(this,c,location,**inputs):
        n=this.node('spawn',**{'class':c}); this.feed(n,SpawnTransform=this.call('MakeTransform',M,Location=location,Rotation='0,0,0',Scale='1,1,1'),CollisionHandlingOverride='DontSpawnIfColliding',**inputs); this.execnode(n); return this.ref(n)
    def binary(this,f,a,b): return this.call(f,M,A=a,B=b)
    def add(this,a,b): return this.binary('Add_DoubleDouble',a,b)
    def sub(this,a,b): return this.binary('Subtract_DoubleDouble',a,b)
    def mul(this,a,b): return this.binary('Multiply_DoubleDouble',a,b)
    def div(this,a,b): return this.binary('Divide_DoubleDouble',a,b)
    def gt(this,a,b): return this.binary('Greater_DoubleDouble',a,b)
    def ge(this,a,b): return this.binary('GreaterEqual_DoubleDouble',a,b)
    def lt(this,a,b): return this.binary('Less_DoubleDouble',a,b)
    def le(this,a,b): return this.binary('LessEqual_DoubleDouble',a,b)
    def eq(this,a,b): return this.binary('EqualEqual_IntInt',a,b)
    def ie(this,a,b): return this.binary('Add_IntInt',a,b)
    def im(this,a,b): return this.binary('Multiply_IntInt',a,b)
    def ne(this,a): return this.call('Not_PreBool',M,A=a)
    def both(this,*xs):
        r=xs[0]
        for x in xs[1:]: r=this.binary('BooleanAND',r,x)
        return r
    def either(this,a,b): return this.binary('BooleanOR',a,b)
    def valid(this,a): return this.call('IsValid',K,Object=a)
    def loc(this,a=None): return this.call('K2_GetActorLocation',ACT,**({'self':a} if a is not None else {}))
    def vadd(this,a,b): return this.binary('Add_VectorVector',a,b)
    def vsub(this,a,b): return this.binary('Subtract_VectorVector',a,b)
    def vmul(this,a,b): return this.binary('Multiply_VectorFloat',a,b)
    def dist(this,a,b): return this.call('Vector_Distance',M,V1=a,V2=b)
    def norm(this,a): return this.call('Vector_Normal2D',M,A=a,Tolerance=.0001)
    def clamp(this,x,lo,hi): return this.call('FClamp',M,Value=x,Min=lo,Max=hi)
    def max(this,a,b): return this.call('FMax',M,A=a,B=b)
    def movement(this,a=None): return this.get('CharacterMovement',a,CH) if a is not None else this.get('CharacterMovement',owner=CH)
    def mesh(this): return this.get('Mesh',owner=CH)
    def health(this): return this.get('Health')
    def hp(this,v): return this.get(v,this.health(),HEALTH)
    def state(this,n): this.do('SetState',NewState=n)
    def play(this,asset,rate=1): this.do('PlayAnimMontage',CH,AnimMontage=asset,InPlayRate=rate,StartSectionName='None')
    def stop(this):
        controller=this.call('GetController',PAWN)
        this.do('StopMovement',CTRL,self=controller)
        this.do('StopMovementImmediately','/Script/Engine.MovementComponent',self=this.movement())
    def face(this,target):
        this.do('K2_SetActorRotation',ACT,NewRotation=this.call('FindLookAtRotation',M,Start=this.loc(),Target=this.loc(target)),bTeleportPhysics=False)

assets=[]
def asset(name,parent,variables,**kw):
    a=dict(path=ROOT+'/'+name,parent=parent,variables=variables,graphs=[],**kw); assets.append(a); return a
def fun(a,name,inputs=(),outputs=(),pure=False):
    g=Graph(name,inputs,outputs,pure); a['graphs'].append(g.d); return g
def eventgraph(a): return fun(a,'EventGraph')
def widget(name,z,widget,hidden=False):
    return dict(name=name,**{'class':WIDGET},properties={'RelativeLocation':f'(X=0,Y=0,Z={z})','Space':'Screen','WidgetClass':widget,'DrawSize':'(X=80,Y=30)' if name=='HealthWidget' else '(X=70,Y=70)','bHiddenInGame':val(hidden)})

# Independent health component. No modified shared-component router is inherited.
h=asset('AC_EnemyHealth_BP','/Script/Engine.ActorComponent',[
    var('MaximumHealth',default=100),var('CurrentHealth',default=100),var('FinisherThreshold',default=.3),var('RatioCache',default=1),var('CanFinish','bool',False),var('Dead','bool',False)])
g=fun(h,'Initialize'); g.set('CurrentHealth',g.get('MaximumHealth')); g.set('Dead',False)
g=fun(h,'UpdateMaximum',[arg('NewMaximum'),arg('Refill','bool')]); g.set('RatioCache',g.div(g.get('CurrentHealth'),g.max(1,g.get('MaximumHealth')))); g.set('MaximumHealth',g.max(1,g.inp('NewMaximum')))
g.branch(g.ne(g.get('Dead')),lambda:g.branch(g.inp('Refill'),lambda:g.set('CurrentHealth',g.get('MaximumHealth')),lambda:g.set('CurrentHealth',g.mul(g.get('MaximumHealth'),g.get('RatioCache')))))
g=fun(h,'ProcessDamage',[arg('Amount')]); g.branch(g.both(g.ne(g.get('Dead')),g.gt(g.inp('Amount'),0)),lambda:g.set('CurrentHealth',g.clamp(g.sub(g.get('CurrentHealth'),g.inp('Amount')),0,g.get('MaximumHealth')))); g.set('Dead',g.le(g.get('CurrentHealth'),0)); g.set('CanFinish',g.both(g.ne(g.get('Dead')),g.le(g.get('CurrentHealth'),g.mul(g.get('MaximumHealth'),g.get('FinisherThreshold')))))

# Point-damage path preserves the existing player's block/health handling.
c=asset('AC_EnemyCombat_BP','/Script/Engine.ActorComponent',[])
g=fun(c,'DealPointDamage',[arg('Victim','object',**{'class':ACT}),arg('Amount')]); victim=g.inp('Victim'); owner=g.call('GetOwner','/Script/Engine.ActorComponent')
g.branch(g.valid(victim),lambda:g.do('ApplyPointDamage',GS,DamagedActor=victim,BaseDamage=g.inp('Amount'),HitFromDirection=g.norm(g.vsub(g.loc(victim),g.loc(owner))),HitInfo=g.call('MakeHitResult',GS,bBlockingHit=True,HitActor=victim,Location=g.loc(victim),ImpactPoint=g.loc(victim),HitComponent=g.cast(g.get('RootComponent',victim,ACT),PRIM)),EventInstigator=g.call('GetInstigatorController',ACT,self=owner),DamageCauser=owner))

# Animation graph has an engine AnimInstance parent and a Blueprint velocity update.
an=asset('ABP_Enemy','/Script/Engine.AnimInstance',[var('Speed','float',0)],animation=True,mesh=NURSE,blendspace='/Game/Blueprints/Agents/Enemies/Animations/AI/BS_EnemyWalkRun.BS_EnemyWalkRun')
g=eventgraph(an); g.event('BlueprintUpdateAnimation',ANIM); pawn=g.call('TryGetPawnOwner',ANIM)
g.branch(g.valid(pawn),lambda:g.set('Speed',g.call('VSizeXY',M,A=g.call('GetVelocity',ACT,self=pawn))))

# Input and timed movement stun are Blueprint actor logic, one instance per local player.
b=asset('BP_PlayerBridge',ACT,[ob('Player',CH),ob('PlayerController',PC),var('StunRemaining'),var('StunLocked','bool',False)])
g=eventgraph(b); g.event('ReceiveBeginPlay',ACT); g.set('Player',g.call('GetPlayerCharacter',GS,PlayerIndex=0)); g.set('PlayerController',g.call('GetPlayerController',GS,PlayerIndex=0)); g.do('EnableInput',ACT,PlayerController=g.get('PlayerController'))
g.event('ReceiveTick',ACT); tick=g.d['nodes'][-1]['id']; g.branch(g.get('StunLocked'),lambda:None)
# The branch above is connected to the event; append the actual timer path.
g.set('StunRemaining',g.max(0,g.sub(g.get('StunRemaining'),g.ref(tick,'DeltaSeconds')))); g.branch(g.both(g.get('StunLocked'),g.le(g.get('StunRemaining'),0)),lambda:(g.do('SetIgnoreMoveInput',CTRL,self=g.get('PlayerController'),bNewMoveInput=False),g.set('StunLocked',False)))
g.event('ReceiveEndPlay',ACT); g.branch(g.get('StunLocked'),lambda:g.do('SetIgnoreMoveInput',CTRL,self=g.get('PlayerController'),bNewMoveInput=False))
g=fun(b,'ApplyStun',[arg('Seconds')]); g.branch(g.both(g.valid(g.get('PlayerController')),g.gt(g.inp('Seconds'),0)),lambda:(g.branch(g.ne(g.get('StunLocked')),lambda:(g.do('SetIgnoreMoveInput',CTRL,self=g.get('PlayerController'),bNewMoveInput=True),g.set('StunLocked',True))),g.set('StunRemaining',g.max(g.get('StunRemaining'),g.inp('Seconds'))),g.do('StopMovementImmediately','/Script/Engine.MovementComponent',self=g.movement(g.get('Player')))))

e=asset('BP_Enemy',CH,[
    var('State','int',0,category='Runtime - 0 Patrol 1 Chase 2 Search 3 Attack 4 Block 5 Finisher 6 Dead'),
    ob('Target',CH),ob('Player',CH),ob('InputBridge',BRIDGE),ob('FinishingPlayer',CH),
    var('DamageLevel','int',0),var('BlockLevel','int',0),var('HealthLevel','int',0),var('MovementLevel','int',0),
    var('HeavyDecisionChance',default=.35),var('BlockDecisionChance',default=.25),var('IntelligenceLevelForMaximum',default=10),var('IntelligenceLevel'),var('IntelligenceAlpha'),var('DecisionInterval',default=.35),var('PathUpdateInterval',default=.4),
    var('BaseHealth',default=100),var('HealthPerLevel',default=20),var('DamagePerLevel',default=.15),var('MovementPerLevel',default=.08),var('BaseBlockHits','int',3),var('BlockHitsPerLevel','int',1),var('MaximumBlockHits','int',3),var('OldBlockCapacity','int',3),
    var('PatrolSpeed',default=160),var('ChaseSpeed',default=360),var('MaximumSpeed',default=750),var('PatrolRadius',default=900),var('PatrolPause',default=1.5),var('SearchDuration',default=10),var('SearchRadius',default=500),
    var('SightRadius',default=1800),var('SightHalfAngle',default=65),var('ProximityRadius',default=250),var('ProximitySeconds',default=2.5),var('SightMemory',default=2),
    var('EnableLight','bool',True),var('EnableHeavy','bool',True),var('EnableTaser','bool',True),var('EnableBlock','bool',True),
    var('LightDamage',default=12),var('HeavyDamage',default=25),var('TaserDamage',default=8),var('MeleeRange',default=160),var('TaserRange',default=1100),var('TaserMinimumRange',default=230),var('LightRate',default=1.35),var('HeavyRate',default=.75),var('HeavyChance',default=.35),var('MeleeWindup',default=.5),var('MeleeRecovery',default=.8),var('TaserWindup',default=.45),var('TaserShotInterval',default=1),var('TaserCooldown',default=6),var('TaserStunSeconds',default=1.25),var('BlockDuration',default=1.2),var('BlockCooldown',default=6),var('BlockChance',default=.25),var('BlockHalfAngle',default=100),
    var('TaserShots','int',2),var('BlockHits','int',3),var('TaserRecharge'),var('BlockRecharge'),var('ProximityTime'),var('UnseenTime'),var('SearchTime'),var('PauseTime'),var('DecisionTime'),var('RepathTime'),var('ActionTime'),var('ImpactTime'),var('BeamTime'),var('FinisherTime'),var('PreviousPlayerMode','enum','MOVE_Walking',enum='/Script/Engine.EMovementMode'),
    var('AttackIndex','int',0),var('AttackAccepted','bool',False),var('BlockAccepted','bool',False),var('PendingImpact','bool',False),var('Visible','bool',False),var('ClearSight','bool',False),var('HasGoal','bool',False),var('NearPlayer','bool',False),var('IncomingDamage'),var('HitWasBlocked','bool',False),var('ShotDamage'),var('AnimationRate',default=1),var('MinimumTaserDistance'),
    var('PursuitGoal','vector','0,0,0'),var('PatrolOrigin','vector','0,0,0'),var('LastKnownLocation','vector','0,0,0'),var('MoveGoal','vector','0,0,0'),var('BeamStart','vector','0,0,0'),var('BeamEnd','vector','0,0,0'),
    ob('BlockAnimation','/Script/Engine.AnimSequence',BASE+'/AS_BlockPlaceholder.AS_BlockPlaceholder'),
    ob('HitReaction','/Script/Engine.AnimMontage',BASE+'/Animations/Progress_EnemyHitReactionSide_Montage.Progress_EnemyHitReactionSide_Montage'),
    ob('EnemyFinisher','/Script/Engine.AnimMontage',BASE+'/Animations/Progress_EnemyFinisherStruggleSuperrMontageGayMan.Progress_EnemyFinisherStruggleSuperrMontageGayMan'),
    ob('PlayerFinisher','/Script/Engine.AnimMontage','/Game/Blueprints/CombatSystem/Animations/Superhuman_Choke_Lift_Montage.Superhuman_Choke_Lift_Montage'),
    ob('GuardMontage','/Script/Engine.AnimMontage'),
    var('MeleeMontages','object','("/Game/Blueprints/Agents/Enemies/Animations/AI/Cross_Punch_Montage.Cross_Punch_Montage","/Game/Blueprints/Agents/Enemies/Animations/AI/Hook_Punch_Montage.Hook_Punch_Montage","/Game/Blueprints/Agents/Enemies/Animations/AI/Punching_Montage.Punching_Montage")',array=True,**{'class':'/Script/Engine.AnimMontage'})
],mesh=NURSE,animclass=cls('ABP_Enemy'),defaults={'AIControllerClass':AIC,'AutoPossessAI':'PlacedInWorldOrSpawned','bUseControllerRotationYaw':'False','bCanBeDamaged':'True','Tags':'("Enemy","BlueprintOnlyEnemy")','PrimaryActorTick':'(bCanEverTick=True,bStartWithTickEnabled=True,TickInterval=0.05)'},mesh_properties={'RelativeLocation':'(X=0,Y=0,Z=-90)','RelativeRotation':'(Pitch=0,Yaw=260,Roll=0)'},movement={'bOrientRotationToMovement':'True','MaxWalkSpeed':'160','RotationRate':'(Pitch=0,Yaw=400,Roll=0)'},components=[
    dict(name='Health',**{'class':HEALTH}),dict(name='Combat',**{'class':COMBAT}),
    widget('HealthWidget',125,HB),widget('FinishWidget',155,'/Game/Blueprints/CombatSystem/Combat/Finisher/WBP_Finish.WBP_Finish_C',True),
    dict(name='TaserBeam',**{'class':SM},properties={'StaticMesh':'/Engine/BasicShapes/Cylinder.Cylinder','bHiddenInGame':'True','CastShadow':'False','bAbsoluteLocation':'True','bAbsoluteRotation':'True','bAbsoluteScale':'True'})])

g=fun(e,'AttackEnabled',[arg('Index','int')],[arg('Enabled','bool')],True)
r=g.either(g.both(g.eq(g.inp('Index'),0),g.get('EnableLight')),g.either(g.both(g.eq(g.inp('Index'),1),g.get('EnableHeavy')),g.both(g.eq(g.inp('Index'),2),g.get('EnableTaser')))); g.feed_result({'Enabled':r})
g=fun(e,'SetState',[arg('NewState','int')]); g.branch(g.ne(g.eq(g.get('State'),g.inp('NewState'))),lambda:(g.branch(g.eq(g.get('State'),4),lambda:g.do('Montage_Stop',ANIM,self=g.call('GetAnimInstance',SK,self=g.mesh()),InBlendOutTime=.1,Montage=g.get('GuardMontage'))),g.set('State',g.inp('NewState')),g.branch(g.eq(g.inp('NewState'),1),lambda:g.set('DecisionTime',g.get('DecisionInterval'))),g.set('HasGoal',False),g.set('RepathTime',0),g.branch(g.eq(g.inp('NewState'),2),lambda:g.set('SearchTime',0))))
g=fun(e,'SetDifficultyLevels',[arg('Damage','int'),arg('Block','int'),arg('HealthLevelInput','int'),arg('MovementSpeed','int')]); g.set('OldBlockCapacity',g.get('MaximumBlockHits'))
for v,p in [('DamageLevel','Damage'),('BlockLevel','Block'),('HealthLevel','HealthLevelInput'),('MovementLevel','MovementSpeed')]: g.set(v,g.call('Clamp',M,Value=g.inp(p),Min=0,Max=10000))
g.do('RefreshIntelligence')
g.do('UpdateMaximum',HEALTH,self=g.health(),NewMaximum=g.add(g.get('BaseHealth'),g.mul(g.get('HealthPerLevel'),g.get('HealthLevel'))),Refill=False)
g.set('MaximumBlockHits',g.call('Clamp',M,Value=g.ie(g.get('BaseBlockHits'),g.im(g.get('BlockLevel'),g.get('BlockHitsPerLevel'))),Min=1,Max=10000))
g.branch(g.le(g.get('BlockRecharge'),0),lambda:g.set('BlockHits',g.call('Clamp',M,Value=g.ie(g.get('BlockHits'),g.binary('Subtract_IntInt',g.get('MaximumBlockHits'),g.get('OldBlockCapacity'))),Min=0,Max=g.get('MaximumBlockHits'))))

g=fun(e,'RefreshIntelligence')
levels=g.add(g.add(g.get('DamageLevel'),g.get('BlockLevel')),g.add(g.get('HealthLevel'),g.get('MovementLevel')))
g.set('IntelligenceLevel',g.mul(levels,.25)); g.set('IntelligenceAlpha',g.clamp(g.div(g.get('IntelligenceLevel'),g.max(.01,g.get('IntelligenceLevelForMaximum'))),0,1))
g.set('DecisionInterval',g.sub(.35,g.mul(.25,g.get('IntelligenceAlpha')))); g.set('PathUpdateInterval',g.sub(.4,g.mul(.25,g.get('IntelligenceAlpha'))))
g.set('DecisionTime',g.call('FMin',M,A=g.get('DecisionTime'),B=g.get('DecisionInterval'))); g.set('RepathTime',g.call('FMin',M,A=g.get('RepathTime'),B=g.get('PathUpdateInterval')))

g=fun(e,'RefreshCombatChoices')
g.set('HeavyDecisionChance',g.clamp(g.get('HeavyChance'),0,1))
def evaluateopening():
    velocity=g.call('GetVelocity',ACT,self=g.get('Target'))
    opening=g.both(g.le(g.call('VSizeXY',M,A=velocity),75),g.le(g.dist(g.loc(),g.loc(g.get('Target'))),g.mul(g.get('MeleeRange'),.8)))
    base=g.clamp(g.get('HeavyChance'),0,1)
    desired=g.call('SelectFloat',M,A=.8,B=0,bPickA=opening)
    g.set('HeavyDecisionChance',g.add(base,g.mul(g.sub(desired,base),g.get('IntelligenceAlpha'))))
g.branch(g.valid(g.get('Target')),evaluateopening)
wounded=g.sub(1,g.clamp(g.div(g.hp('CurrentHealth'),g.max(1,g.hp('MaximumHealth'))),0,1))
g.set('BlockDecisionChance',g.clamp(g.add(g.get('BlockChance'),g.mul(g.mul(g.get('IntelligenceAlpha'),wounded),.4)),0,.85))

g=fun(e,'SetAttackEnabled',[arg('Index','int'),arg('Enabled','bool')])
for i,v in enumerate(['EnableLight','EnableHeavy','EnableTaser']): g.branch(g.eq(g.inp('Index'),i),lambda v=v:g.set(v,g.inp('Enabled')))
g.branch(g.both(g.eq(g.get('State'),3),g.eq(g.get('AttackIndex'),g.inp('Index')),g.ne(g.inp('Enabled'))),lambda:(g.set('PendingImpact',False),g.set('ActionTime',0),g.do('StopAnimMontage',CH),g.state(1)))

g=fun(e,'ClearLineTo',[arg('Actor','object',**{'class':ACT})],[arg('Clear','bool')]); g.set('ClearSight',False)
def clearline():
    start=g.vadd(g.loc(),'0,0,64'); end=g.vadd(g.loc(g.inp('Actor')),'0,0,45')
    t=g.do('LineTraceSingle',K,Start=start,End=end,TraceChannel='TraceTypeQuery1',bTraceComplex=False,ActorsToIgnore=g.array([g.self()]),DrawDebugType='None',bIgnoreSelf=True)
    hit=g.call('BreakHitResult',GS,Hit=g.ref(t,'OutHit'),out='HitActor')
    g.set('ClearSight',g.either(g.ne(g.ref(t)),g.binary('EqualEqual_ObjectObject',hit,g.inp('Actor'))))
g.branch(g.valid(g.inp('Actor')),clearline); g.end(Clear=g.get('ClearSight'))

g=fun(e,'NotifyPlayerDamage',[arg('Source','object',**{'class':ACT})]); source=g.cast(g.inp('Source'),CH)
def notice():
    g.branch(g.call('IsPlayerControlled',PAWN,self=source),lambda:(g.set('Target',source),g.set('LastKnownLocation',g.loc(source)),g.set('UnseenTime',0),g.branch(g.either(g.lt(g.get('State'),3),g.eq(g.get('State'),2)),lambda:g.state(1))))
g.branch(g.both(g.valid(source),g.lt(g.get('State'),5)),notice)

g=fun(e,'MoveToPoint',[arg('Goal','vector'),arg('Acceptance')]);
def move():
    g.set('RepathTime',g.get('PathUpdateInterval')); g.set('MoveGoal',g.inp('Goal')); g.set('HasGoal',True)
    g.branch(g.gt(g.dist(g.loc(),g.inp('Goal')),g.inp('Acceptance')),lambda:g.do('SimpleMoveToLocation',AI,Controller=g.call('GetController',PAWN),Goal=g.inp('Goal')),lambda:g.stop())
g.branch(g.le(g.get('RepathTime'),0),move)

g=fun(e,'TryBlock',outputs=[arg('Started','bool')]); g.set('BlockAccepted',False)
def block():
    g.stop(); g.state(4); g.set('ActionTime',g.get('BlockDuration')); g.set('BlockAccepted',True)
    n=g.do('PlaySlotAnimationAsDynamicMontage',ANIM,self=g.call('GetAnimInstance',SK,self=g.mesh()),Asset=g.get('BlockAnimation'),SlotNodeName='DefaultSlot',BlendInTime=.1,BlendOutTime=.1,InPlayRate=1,LoopCount=100,BlendOutTriggerTime=-1,InTimeToStartMontageAt=0); g.set('GuardMontage',g.ref(n))
g.branch(g.both(g.eq(g.get('State'),1),g.get('EnableBlock'),g.gt(g.get('BlockHits'),0),g.le(g.get('BlockRecharge'),0)),block); g.end(Started=g.get('BlockAccepted'))

g=fun(e,'TryAttack',[arg('Index','int')],[arg('Started','bool')]); g.set('AttackAccepted',False)
def attackvalid():
    t=g.do('ClearLineTo',Actor=g.get('Target'))
    g.set('MinimumTaserDistance',g.get('TaserMinimumRange')); g.branch(g.both(g.ne(g.get('EnableLight')),g.ne(g.get('EnableHeavy'))),lambda:g.set('MinimumTaserDistance',0))
    d=g.dist(g.loc(),g.loc(g.get('Target')))
    melee=g.both(g.lt(g.inp('Index'),2),g.le(d,g.get('MeleeRange')))
    ranged=g.both(g.eq(g.inp('Index'),2),g.le(d,g.get('TaserRange')),g.ge(d,g.get('MinimumTaserDistance')),g.gt(g.get('TaserShots'),0),g.le(g.get('TaserRecharge'),0))
    def start():
        g.stop(); g.face(g.get('Target')); g.set('AttackIndex',g.inp('Index')); g.set('AnimationRate',g.get('LightRate')); g.branch(g.eq(g.inp('Index'),1),lambda:g.set('AnimationRate',g.get('HeavyRate')))
        g.branch(g.eq(g.inp('Index'),2),lambda:(g.set('ImpactTime',g.get('TaserWindup')),g.set('ActionTime',g.add(g.get('TaserWindup'),g.get('TaserShotInterval')))),lambda:(g.set('ImpactTime',g.div(g.get('MeleeWindup'),g.max(.01,g.get('AnimationRate')))),g.set('ActionTime',g.div(g.add(g.get('MeleeWindup'),g.get('MeleeRecovery')),g.max(.01,g.get('AnimationRate'))))))
        g.set('PendingImpact',True); g.set('AttackAccepted',True); g.state(3)
        def punch():
            pick=g.call('Array_Get',AR,TargetArray=g.get('MeleeMontages'),Index=g.call('RandomIntegerInRange',M,Min=0,Max=2),out='Item'); g.play(pick,g.get('AnimationRate'))
        g.branch(g.lt(g.inp('Index'),2),punch)
    g.branch(g.both(g.ref(t,'Clear'),g.either(melee,ranged)),start)
g.branch(g.both(g.eq(g.get('State'),1),g.valid(g.get('Target')),g.call('AttackEnabled',Index=g.inp('Index'),out='Enabled')),attackvalid); g.end(Started=g.get('AttackAccepted'))

g=fun(e,'UpdateResources',[arg('Delta')])
for timer,charge,refill in [('TaserRecharge','TaserShots',2),('BlockRecharge','BlockHits',None)]:
    def recharge(timer=timer,charge=charge,refill=refill):
        g.set(timer,g.max(0,g.sub(g.get(timer),g.inp('Delta')))); g.branch(g.le(g.get(timer),0),lambda:g.set(charge,g.get('MaximumBlockHits') if refill is None else refill))
    g.branch(g.gt(g.get(timer),0),recharge)
for timer in ['DecisionTime','RepathTime','BeamTime']:
    g.set(timer,g.max(0,g.sub(g.get(timer),g.inp('Delta'))))
g.branch(g.le(g.get('BeamTime'),0),lambda:g.do('SetHiddenInGame',SCENE,self=g.get('TaserBeam'),NewHidden=True,bPropagateToChildren=False))

g=fun(e,'ResolveAttack'); g.set('PendingImpact',False)
def resolve():
    taser=g.eq(g.get('AttackIndex'),2)
    g.branch(taser,lambda:(g.set('TaserShots',g.binary('Subtract_IntInt',g.get('TaserShots'),1)),g.branch(g.le(g.get('TaserShots'),0),lambda:g.set('TaserRecharge',g.get('TaserCooldown')))))
    g.set('ShotDamage',g.get('LightDamage')); g.branch(g.eq(g.get('AttackIndex'),1),lambda:g.set('ShotDamage',g.get('HeavyDamage'))); g.branch(taser,lambda:g.set('ShotDamage',g.get('TaserDamage')))
    t=g.do('ClearLineTo',Actor=g.get('Target'))
    # Beam presentation uses an engine static-mesh component, not debug-only lines.
    def beam():
        g.set('BeamStart',g.vadd(g.loc(),'0,0,64')); g.set('BeamEnd',g.vadd(g.loc(g.get('Target')),'0,0,45'))
        direction=g.vsub(g.get('BeamEnd'),g.get('BeamStart'))
        g.do('K2_SetWorldLocation',SCENE,self=g.get('TaserBeam'),NewLocation=g.vmul(g.vadd(g.get('BeamStart'),g.get('BeamEnd')),.5),bSweep=False,bTeleport=True)
        g.do('K2_SetWorldRotation',SCENE,self=g.get('TaserBeam'),NewRotation=g.call('MakeRotFromZ',M,Z=direction),bSweep=False,bTeleport=True)
        scale=g.call('MakeVector',M,X=.025,Y=.025,Z=g.div(g.call('VSize',M,A=direction),100)); g.do('SetWorldScale3D',SCENE,self=g.get('TaserBeam'),NewScale=scale)
        g.do('SetHiddenInGame',SCENE,self=g.get('TaserBeam'),NewHidden=False,bPropagateToChildren=False); g.set('BeamTime',.15)
    g.branch(taser,beam)
    distance=g.dist(g.loc(),g.loc(g.get('Target'))); front=g.ge(g.binary('Dot_VectorVector',g.call('GetActorForwardVector',ACT),g.norm(g.vsub(g.loc(g.get('Target')),g.loc()))),.25)
    inrange=g.either(g.both(taser,g.le(distance,g.get('TaserRange'))),g.both(g.ne(taser),g.le(distance,g.get('MeleeRange')),front))
    def hit():
        g.do('DealPointDamage',COMBAT,self=g.get('Combat'),Victim=g.get('Target'),Amount=g.mul(g.get('ShotDamage'),g.add(1,g.mul(g.get('DamagePerLevel'),g.get('DamageLevel')))))
        def stun():
            player=g.cast(g.get('Target'),PLAYER)
            g.branch(g.valid(player),lambda:g.branch(g.ne(g.get('Blocking',player,PLAYER)),lambda:g.do('ApplyStun',BRIDGE,self=g.get('InputBridge'),Seconds=g.get('TaserStunSeconds'))))
        g.branch(g.both(taser,g.valid(g.get('InputBridge'))),stun)
    g.branch(g.both(g.ref(t,'Clear'),inrange),hit)
g.branch(g.both(g.valid(g.get('Target')),g.call('AttackEnabled',Index=g.get('AttackIndex'),out='Enabled')),resolve)

g=fun(e,'UpdateSenses',[arg('Delta')]); g.set('Player',g.call('GetPlayerCharacter',GS,PlayerIndex=0)); g.set('Visible',False); g.set('NearPlayer',False)
def senses():
    t=g.do('ClearLineTo',Actor=g.get('Player')); offset=g.vsub(g.loc(g.get('Player')),g.loc()); distance=g.call('VSize',M,A=offset)
    cone=g.ge(g.binary('Dot_VectorVector',g.call('GetActorForwardVector',ACT),g.norm(offset)),g.call('DegCos',M,A=g.get('SightHalfAngle')))
    g.set('Visible',g.both(g.ref(t,'Clear'),cone,g.le(distance,g.get('SightRadius')))); g.set('NearPlayer',g.both(g.ref(t,'Clear'),g.le(distance,g.get('ProximityRadius'))))
g.branch(g.valid(g.get('Player')),lambda:g.branch(g.get('bCanBeDamaged',g.get('Player'),ACT),senses))
g.branch(g.get('NearPlayer'),lambda:g.set('ProximityTime',g.add(g.get('ProximityTime'),g.inp('Delta'))),lambda:g.set('ProximityTime',0))
def aware():
    g.set('Target',g.get('Player')); g.set('LastKnownLocation',g.loc(g.get('Player'))); g.set('UnseenTime',0); g.branch(g.either(g.eq(g.get('State'),0),g.eq(g.get('State'),2)),lambda:g.state(1))
g.branch(g.either(g.get('Visible'),g.ge(g.get('ProximityTime'),g.get('ProximitySeconds'))),aware,lambda:g.set('UnseenTime',g.add(g.get('UnseenTime'),g.inp('Delta'))))

g=fun(e,'PatrolOrSearch',[arg('Delta')]); g.branch(g.eq(g.get('State'),2),lambda:(g.set('SearchTime',g.add(g.get('SearchTime'),g.inp('Delta'))),g.branch(g.ge(g.get('SearchTime'),g.get('SearchDuration')),lambda:(g.set('Target',None),g.state(0)))))
def reached(): g.set('HasGoal',False); g.set('PauseTime',g.get('PatrolPause'))
controller=g.cast(g.call('GetController',PAWN),AIC)
g.branch(g.get('HasGoal'),lambda:g.branch(g.either(g.lt(g.dist(g.loc(),g.get('MoveGoal')),100),g.binary('EqualEqual_ByteByte',g.call('GetMoveStatus',AIC,self=controller),'Idle')),reached))
def choose():
    g.set('PauseTime',g.sub(g.get('PauseTime'),g.inp('Delta')))
    def pick():
        g.set('MoveGoal',g.get('PatrolOrigin')); g.set('ShotDamage',g.get('PatrolRadius'))
        g.branch(g.eq(g.get('State'),2),lambda:(g.set('MoveGoal',g.get('LastKnownLocation')),g.set('ShotDamage',g.get('SearchRadius'))))
        n=g.do('K2_GetRandomReachablePointInRadius',NAV,force=True,Origin=g.get('MoveGoal'),Radius=g.get('ShotDamage'))
        g.branch(g.ref(n),lambda:g.do('MoveToPoint',Goal=g.ref(n,'RandomLocation'),Acceptance=60),lambda:g.set('PauseTime',1))
    g.branch(g.le(g.get('PauseTime'),0),pick)
g.branch(g.ne(g.get('HasGoal')),choose)

g=fun(e,'Think',[arg('Delta')])
def chase():
    g.branch(g.valid(g.get('Target')),lambda:g.branch(g.either(g.get('Visible'),g.ge(g.get('ProximityTime'),g.get('ProximitySeconds'))),fight,searchlast),lambda:g.state(2))
def fight():
    def decide():
        g.do('RefreshCombatChoices')
        distance=g.dist(g.loc(),g.loc(g.get('Target')))
        def melee():
            g.branch(g.both(g.get('EnableBlock'),g.call('RandomBoolWithWeight',M,Weight=g.get('BlockDecisionChance'))),lambda:g.do('TryBlock'))
            g.branch(g.eq(g.get('State'),1),lambda:(g.set('AttackIndex',0),g.branch(g.both(g.get('EnableHeavy'),g.either(g.ne(g.get('EnableLight')),g.call('RandomBoolWithWeight',M,Weight=g.get('HeavyDecisionChance')))),lambda:g.set('AttackIndex',1)),g.do('TryAttack',Index=g.get('AttackIndex'))))
        g.branch(g.le(distance,g.get('MeleeRange')),melee)
        g.branch(g.eq(g.get('State'),1),lambda:g.do('TryAttack',Index=2)); g.set('DecisionTime',g.get('DecisionInterval'))
    g.branch(g.le(g.get('DecisionTime'),0),decide)
    g.set('PursuitGoal',g.get('LastKnownLocation'))
    lead=g.vmul(g.call('Vector_ClampSizeMax',M,A=g.call('GetVelocity',ACT,self=g.get('Target')),Max=450),g.mul(.45,g.get('IntelligenceAlpha')))
    g.branch(g.get('Visible'),lambda:g.set('PursuitGoal',g.vadd(g.get('LastKnownLocation'),lead)))
    g.branch(g.eq(g.get('State'),1),lambda:g.do('MoveToPoint',Goal=g.get('PursuitGoal'),Acceptance=g.max(30,g.mul(g.get('MeleeRange'),.65))))
def searchlast():
    g.do('MoveToPoint',Goal=g.get('LastKnownLocation'),Acceptance=70)
    lost=g.either(g.both(g.ge(g.get('UnseenTime'),g.get('SightMemory')),g.lt(g.dist(g.loc(),g.get('LastKnownLocation')),150)),g.gt(g.get('UnseenTime'),g.add(g.get('SightMemory'),8)))
    g.branch(lost,lambda:g.state(2))
g.branch(g.eq(g.get('State'),1),chase,lambda:g.do('PatrolOrSearch',Delta=g.inp('Delta')))

g=fun(e,'UpdateBrain',[arg('Delta')])
def brain():
    g.do('UpdateSenses',Delta=g.inp('Delta'))
    def attacking():
        g.set('ActionTime',g.sub(g.get('ActionTime'),g.inp('Delta'))); g.set('ImpactTime',g.sub(g.get('ImpactTime'),g.inp('Delta')))
        g.branch(g.both(g.get('PendingImpact'),g.le(g.get('ImpactTime'),0)),lambda:g.do('ResolveAttack'))
        g.branch(g.le(g.get('ActionTime'),0),lambda:g.state(1))
    def blocking():
        g.stop(); g.branch(g.valid(g.get('Target')),lambda:g.face(g.get('Target'))); g.set('ActionTime',g.sub(g.get('ActionTime'),g.inp('Delta')))
        end=g.either(g.ne(g.get('EnableBlock')),g.either(g.le(g.get('BlockHits'),0),g.le(g.get('ActionTime'),0)))
        g.branch(end,lambda:(g.state(1),g.set('DecisionTime',g.mul(g.get('DecisionInterval'),5/3.5))))
    def thinking():
        g.set('AnimationRate',g.get('PatrolSpeed')); g.branch(g.eq(g.get('State'),1),lambda:g.set('AnimationRate',g.get('ChaseSpeed')))
        speed=g.clamp(g.mul(g.get('AnimationRate'),g.add(1,g.mul(g.get('MovementPerLevel'),g.get('MovementLevel')))),1,g.get('MaximumSpeed'))
        g.set('MaxWalkSpeed',speed,g.movement(),MOVE); g.do('Think',Delta=g.inp('Delta'))
    g.branch(g.eq(g.get('State'),3),attacking,lambda:g.branch(g.eq(g.get('State'),4),blocking,thinking))
g.branch(g.lt(g.get('State'),5),brain)

g=fun(e,'UpdatePresentation')
bar=g.cast(g.call('GetUserWidgetObject',WIDGET,self=g.get('HealthWidget')),HB)
g.branch(g.valid(bar),lambda:g.branch(g.valid(g.get('Material',bar,HB)),lambda:g.do('SetFillLevel',HB,self=bar,Level=g.div(g.hp('CurrentHealth'),g.max(1,g.hp('MaximumHealth'))))))
g.set('CanFinish',g.both(g.gt(g.hp('CurrentHealth'),0),g.le(g.hp('CurrentHealth'),g.mul(g.hp('MaximumHealth'),g.hp('FinisherThreshold'))),g.lt(g.get('State'),5)),g.health(),HEALTH)
g.do('SetHiddenInGame',SCENE,self=g.get('FinishWidget'),NewHidden=g.ne(g.hp('CanFinish')),bPropagateToChildren=False)

g=fun(e,'ReleaseFinishingPlayer')
def release():
    player=g.cast(g.get('FinishingPlayer'),PLAYER); g.branch(g.valid(player),lambda:g.set('FinisherActive',False,player,PLAYER))
    g.do('SetMovementMode',MOVE,self=g.movement(g.get('FinishingPlayer')),NewMovementMode=g.get('PreviousPlayerMode'),NewCustomMode=0)
    g.set('FinishingPlayer',None)
g.branch(g.valid(g.get('FinishingPlayer')),release)

g=fun(e,'Die');
def die():
    g.do('ReleaseFinishingPlayer'); g.stop(); g.state(6); g.set('PendingImpact',False); g.set('bCanBeDamaged',False,owner=ACT); g.do('DisableMovement',MOVE,self=g.movement()); g.do('StopAnimMontage',CH)
    capsule=g.get('CapsuleComponent',owner=CH); g.do('SetCollisionEnabled',PRIM,self=capsule,NewType='NoCollision'); g.do('SetCollisionProfileName',PRIM,self=g.mesh(),InCollisionProfileName='Ragdoll',bUpdateOverlaps=True); g.do('SetSimulatePhysics',PRIM,self=g.mesh(),bSimulate=True)
    for widgetname in ['HealthWidget','FinishWidget','TaserBeam']: g.do('SetHiddenInGame',SCENE,self=g.get(widgetname),NewHidden=True,bPropagateToChildren=False)
    sanity=g.do('GetActorOfClass',GS,ActorClass='/Game/ShaderMats/Sanity.Sanity_C'); so=g.cast(g.ref(sanity),'/Game/ShaderMats/Sanity.Sanity_C')
    g.branch(g.valid(so),lambda:g.do('SanityActivation','/Game/ShaderMats/Sanity.Sanity_C',self=so)); g.do('SetLifeSpan',ACT,InLifespan=8)
g.branch(g.ne(g.eq(g.get('State'),6)),die)

g=fun(e,'BeginFinisher',[arg('PlayerActor','object',**{'class':CH})])
def finishing():
    line=g.do('ClearLineTo',Actor=g.inp('PlayerActor'))
    def start():
        g.stop(); g.state(5); g.set('PendingImpact',False); g.set('FinishingPlayer',g.inp('PlayerActor')); g.set('PreviousPlayerMode',g.get('MovementMode',g.movement(g.inp('PlayerActor')),MOVE))
        g.do('DisableMovement',MOVE,self=g.movement()); g.do('DisableMovement',MOVE,self=g.movement(g.inp('PlayerActor')))
        p=g.cast(g.inp('PlayerActor'),PLAYER); g.branch(g.valid(p),lambda:g.set('FinisherActive',True,p,PLAYER))
        g.face(g.inp('PlayerActor')); g.play(g.get('EnemyFinisher')); played=g.do('PlayAnimMontage',CH,self=g.inp('PlayerActor'),AnimMontage=g.get('PlayerFinisher'),InPlayRate=1,StartSectionName='None'); g.set('FinisherTime',g.max(.1,g.ref(played)))
    g.branch(g.ref(line,'Clear'),start)
g.branch(g.both(g.hp('CanFinish'),g.valid(g.inp('PlayerActor')),g.le(g.dist(g.loc(),g.loc(g.inp('PlayerActor'))),350)),finishing)

g=fun(e,'AdvanceFinisher',[arg('Delta')])
def advance():
    g.set('FinisherTime',g.sub(g.get('FinisherTime'),g.inp('Delta')))
    g.branch(g.le(g.get('FinisherTime'),0),lambda:(g.do('ProcessDamage',HEALTH,self=g.health(),Amount=g.hp('MaximumHealth')),g.do('Die')))
g.branch(g.eq(g.get('State'),5),advance)

g=fun(e,'ReceiveCombatDamage',[arg('Amount'),arg('Source','object',**{'class':ACT})])
def damage():
    g.do('NotifyPlayerDamage',Source=g.inp('Source')); g.set('IncomingDamage',g.inp('Amount')); g.set('HitWasBlocked',False)
    def guard():
        dot=g.binary('Dot_VectorVector',g.call('GetActorForwardVector',ACT),g.norm(g.vsub(g.loc(g.inp('Source')),g.loc())))
        def absorb():
            g.set('HitWasBlocked',True); g.set('IncomingDamage',g.mul(g.inp('Amount'),.25)); g.set('BlockHits',g.binary('Subtract_IntInt',g.get('BlockHits'),1)); g.branch(g.le(g.get('BlockHits'),0),lambda:(g.set('BlockRecharge',g.get('BlockCooldown')),g.state(1)))
        g.branch(g.ge(dot,g.call('DegCos',M,A=g.get('BlockHalfAngle'))),absorb)
    g.branch(g.both(g.eq(g.get('State'),4),g.get('EnableBlock'),g.gt(g.get('BlockHits'),0),g.valid(g.inp('Source'))),guard)
    g.do('ProcessDamage',HEALTH,self=g.health(),Amount=g.get('IncomingDamage'))
    def react():
        g.set('PendingImpact',False); g.branch(g.eq(g.get('State'),3),lambda:g.state(1)); g.play(g.get('HitReaction')); g.set('DecisionTime',g.mul(g.get('DecisionInterval'),4/3.5))
    g.branch(g.hp('Dead'),lambda:g.do('Die'),lambda:g.branch(g.ne(g.get('HitWasBlocked')),react)); g.do('UpdatePresentation')
g.branch(g.both(g.lt(g.get('State'),5),g.gt(g.inp('Amount'),0)),damage)

g=eventgraph(e); g.event('ReceiveBeginPlay',ACT)
g.set('PatrolOrigin',g.loc()); g.do('Initialize',HEALTH,self=g.health()); g.do('SetDifficultyLevels',Damage=g.get('DamageLevel'),Block=g.get('BlockLevel'),HealthLevelInput=g.get('HealthLevel'),MovementSpeed=g.get('MovementLevel')); g.do('SpawnDefaultController',PAWN)
g.do('SetCollisionResponseToChannel',PRIM,self=g.get('CapsuleComponent',owner=CH),Channel='ECC_Visibility',NewResponse='ECR_Block')
g.do('SetCollisionEnabled',PRIM,self=g.get('TaserBeam'),NewType='NoCollision'); g.do('SetStaticMesh',SM,self=g.get('TaserBeam'),NewMesh='/Engine/BasicShapes/Cylinder.Cylinder'); g.do('SetMaterial',PRIM,self=g.get('TaserBeam'),ElementIndex=0,Material='/Game/LevelPrototyping/Materials/MI_Solid_Blue.MI_Solid_Blue')
existing=g.do('GetActorOfClass',GS,ActorClass=BRIDGE); bridge=g.cast(g.ref(existing),BRIDGE)
g.branch(g.valid(bridge),lambda:g.set('InputBridge',bridge),lambda:g.set('InputBridge',g.spawn(BRIDGE,'0,0,0'))); g.do('UpdatePresentation')
tick=g.event('ReceiveTick',ACT)
def tickalive():
    g.do('UpdateResources',Delta=g.ref(tick,'DeltaSeconds')); g.do('UpdateBrain',Delta=g.ref(tick,'DeltaSeconds')); g.do('AdvanceFinisher',Delta=g.ref(tick,'DeltaSeconds')); g.do('UpdatePresentation')
g.branch(g.ne(g.eq(g.get('State'),6)),tickalive)
dam=g.event('ReceiveAnyDamage',ACT)
g.branch(g.valid(g.ref(dam,'InstigatedBy')),lambda:g.do('ReceiveCombatDamage',Amount=g.ref(dam,'Damage'),Source=g.call('K2_GetPawn',CTRL,self=g.ref(dam,'InstigatedBy'))),lambda:g.do('ReceiveCombatDamage',Amount=g.ref(dam,'Damage'),Source=g.ref(dam,'DamageCauser')))
g.event('ReceiveEndPlay',ACT); g.do('ReleaseFinishingPlayer')

# Enhanced input finisher path: the original player asset remains untouched.
g=Graph('EventGraph')
# Append to the bridge's already-created EventGraph, preserving its node IDs.
bridge_graph=b['graphs'][0]; g.d=bridge_graph; g.n=max(int(n['id'][1:]) for n in g.d['nodes']); g.tail=[]
press=g.node('input',action='/Game/Input/Actions/IA_Finisher.IA_Finisher'); g.tail=[g.ref(press,'Started')]
cam=g.call('GetPlayerCameraManager',GS,PlayerIndex=0); start=g.call('GetCameraLocation','/Script/Engine.PlayerCameraManager',self=cam); forward=g.call('GetForwardVector',M,InRot=g.call('GetCameraRotation','/Script/Engine.PlayerCameraManager',self=cam))
trace=g.do('LineTraceSingle',K,Start=start,End=g.vadd(start,g.vmul(forward,350)),TraceChannel='TraceTypeQuery1',bTraceComplex=False,ActorsToIgnore=g.array([g.get('Player')]),DrawDebugType='None',bIgnoreSelf=True)
enemy=g.cast(g.call('BreakHitResult',GS,Hit=g.ref(trace,'OutHit'),out='HitActor'),CHAR)
g.branch(g.valid(enemy),lambda:g.do('BeginFinisher',CHAR,self=enemy,PlayerActor=g.get('Player')))

# The Blueprint spawner supports both the procedural dungeon and a plain arena.
s=asset('BP_EnemySpawner',ACT,[
    var('SpawnAroundSpawner','bool',False),var('WaitForMap','bool',True),var('MinimumPerRoom','int',1),var('MaximumPerRoom','int',2),var('MaximumAlive','int',20),var('SpawnRadius',default=650),var('MinimumPlayerDistance',default=900),var('MinimumEnemyDistance',default=180),var('SpawnBudget','int',3),var('AliveCount','int',0),var('SpawnsThisTick','int',0),var('RoomIndex','int',0),
    var('DamageLevel','int',0),var('BlockLevel','int',0),var('HealthLevel','int',0),var('MovementLevel','int',0),var('Ready','bool',False),
    ob('Rooms',ACT,'',array=True),var('RoomCounts','int','',array=True),var('RoomQuotas','int','',array=True),ob('SpawnedEnemies',CHAR,'',array=True),ob('Player',CH),var('Candidate','vector','0,0,0'),
],defaults={'PrimaryActorTick':'(bCanEverTick=True,bStartWithTickEnabled=True,TickInterval=0.5)'})
g=fun(s,'SetDifficultyLevels',[arg('Damage','int'),arg('Block','int'),arg('HealthLevelInput','int'),arg('MovementSpeed','int'),arg('UpdateExisting','bool')])
for v,p in [('DamageLevel','Damage'),('BlockLevel','Block'),('HealthLevel','HealthLevelInput'),('MovementLevel','MovementSpeed')]: g.set(v,g.call('Clamp',M,Value=g.inp(p),Min=0,Max=10000))
g.branch(g.inp('UpdateExisting'),lambda:g.foreach(g.get('SpawnedEnemies'),lambda enemy,index:g.branch(g.valid(enemy),lambda:g.do('SetDifficultyLevels',CHAR,self=enemy,Damage=g.get('DamageLevel'),Block=g.get('BlockLevel'),HealthLevelInput=g.get('HealthLevel'),MovementSpeed=g.get('MovementLevel')))))
g=fun(s,'ProcessRoom',[arg('Room','object',**{'class':ACT})]); g.set('RoomIndex',g.call('Array_Find',AR,TargetArray=g.get('Rooms'),ItemToFind=g.inp('Room')))
def addroom():
    n=g.do('Array_Add',AR,TargetArray=g.get('Rooms'),NewItem=g.inp('Room')); g.set('RoomIndex',g.ref(n)); g.do('Array_Add',AR,TargetArray=g.get('RoomCounts'),NewItem=0)
    quota=g.do('RandomIntegerInRange',M,force=True,Min=g.get('MinimumPerRoom'),Max=g.get('MaximumPerRoom')); g.do('Array_Add',AR,TargetArray=g.get('RoomQuotas'),NewItem=g.ref(quota))
g.branch(g.eq(g.get('RoomIndex'),-1),addroom)
count=g.call('Array_Get',AR,TargetArray=g.get('RoomCounts'),Index=g.get('RoomIndex'),out='Item'); quota=g.call('Array_Get',AR,TargetArray=g.get('RoomQuotas'),Index=g.get('RoomIndex'),out='Item')
def attempt():
    point=g.do('K2_GetRandomReachablePointInRadius',NAV,force=True,Origin=g.loc(g.inp('Room')),Radius=g.get('SpawnRadius')); g.set('Candidate',g.vadd(g.ref(point,'RandomLocation'),'0,0,101'))
    def spawnclear():
        overlap=g.do('CapsuleOverlapActors',K,CapsulePos=g.get('Candidate'),Radius=42,HalfHeight=96,ObjectTypes=g.array(['ObjectTypeQuery1','ObjectTypeQuery3']),ActorClassFilter=ACT,ActorsToIgnore=g.array([g.self()]))
        def spawning():
            born=g.spawn(CHAR,g.get('Candidate'))
            def record():
                g.do('SetDifficultyLevels',CHAR,self=born,Damage=g.get('DamageLevel'),Block=g.get('BlockLevel'),HealthLevelInput=g.get('HealthLevel'),MovementSpeed=g.get('MovementLevel')); g.do('Array_Add',AR,TargetArray=g.get('SpawnedEnemies'),NewItem=born)
                g.do('Array_Set',AR,TargetArray=g.get('RoomCounts'),Index=g.get('RoomIndex'),Item=g.ie(count,1),bSizeToFit=False); g.set('AliveCount',g.ie(g.get('AliveCount'),1)); g.set('SpawnsThisTick',g.ie(g.get('SpawnsThisTick'),1))
            g.branch(g.valid(born),record)
        def separated():
            nearby=g.do('SphereOverlapActors',K,SpherePos=g.get('Candidate'),SphereRadius=g.get('MinimumEnemyDistance'),ObjectTypes=g.array(['ObjectTypeQuery3']),ActorClassFilter=CHAR,ActorsToIgnore=g.array([g.self()]))
            g.branch(g.ne(g.ref(nearby)),spawning)
        g.branch(g.ne(g.ref(overlap)),separated)
    g.branch(g.both(g.ref(point),g.ge(g.dist(g.get('Candidate'),g.loc(g.get('Player'))),g.get('MinimumPlayerDistance'))),spawnclear)
g.branch(g.both(g.lt(count,quota),g.lt(g.get('AliveCount'),g.get('MaximumAlive')),g.lt(g.get('SpawnsThisTick'),g.get('SpawnBudget'))),attempt)
g=fun(s,'SpawnPending'); g.set('Player',g.call('GetPlayerCharacter',GS,PlayerIndex=0)); g.set('Ready',g.either(g.get('SpawnAroundSpawner'),g.ne(g.get('WaitForMap'))))
def readycheck():
    n=g.do('GetActorOfClass',GS,ActorClass='/Game/Blueprints/ProceduralMap/Map_Generator.Map_Generator_C'); gen=g.cast(g.ref(n),'/Game/Blueprints/ProceduralMap/Map_Generator.Map_Generator_C')
    g.branch(g.valid(gen),lambda:g.set('Ready',g.get('MapComplete',gen,'/Game/Blueprints/ProceduralMap/Map_Generator.Map_Generator_C')))
g.branch(g.ne(g.get('Ready')),readycheck)
def population():
    g.set('AliveCount',0); g.set('SpawnsThisTick',0)
    def countalive(enemy,index):
        def valid():
            health=g.get('Health',enemy,CHAR); g.branch(g.ne(g.get('Dead',health,HEALTH)),lambda:g.set('AliveCount',g.ie(g.get('AliveCount'),1)))
        g.branch(g.valid(enemy),valid)
    g.foreach(g.get('SpawnedEnemies'),countalive)
    def dungeon():
        rooms=g.do('GetAllActorsOfClass',GS,ActorClass='/Game/Blueprints/ProceduralMap/Rooms/Master_Room.Master_Room_C')
        def room(r,index):
            # Exclude the player's room using a generous local distance check.
            startroom=g.call('Contains','/Script/Engine.KismetStringLibrary',SearchIn=g.call('GetObjectName',K,Object=r),Substring='Spawn_Room',bUseCase=False,bSearchFromEnd=False)
            g.branch(g.both(g.ne(startroom),g.ge(g.dist(g.loc(r),g.loc(g.get('Player'))),g.get('MinimumPlayerDistance'))),lambda:g.do('ProcessRoom',Room=r))
        g.foreach(g.ref(rooms,'OutActors'),room)
    g.branch(g.get('SpawnAroundSpawner'),lambda:g.do('ProcessRoom',Room=g.self()),dungeon)
g.branch(g.both(g.get('Ready'),g.valid(g.get('Player'))),population)
g=eventgraph(s); g.event('ReceiveTick',ACT); g.do('SpawnPending')

# Optional smoke-test observer is itself a Blueprint. It does nothing during normal Play.
observer=asset('BP_TestObserver',ACT,[var('Enabled','bool',False),var('Elapsed'),var('Reported','bool',False),var('EnemyCount','int',0),var('Controllers','int',0),var('Moving','int',0)])
g=eventgraph(observer); g.event('ReceiveBeginPlay',ACT)
command=g.do('GetCommandLine',K)
flag=g.call('Contains','/Script/Engine.KismetStringLibrary',SearchIn=g.ref(command),Substring='BlueprintOnlySmoke',bUseCase=False,bSearchFromEnd=False); g.set('Enabled',flag)
tick=g.event('ReceiveTick',ACT)
def observe():
    g.set('Elapsed',g.add(g.get('Elapsed'),g.ref(tick,'DeltaSeconds')))
    def report():
        g.set('Reported',True); g.set('Controllers',0); g.set('Moving',0)
        found=g.do('GetAllActorsOfClass',GS,ActorClass=CHAR); g.set('EnemyCount',g.call('Array_Length',AR,TargetArray=g.ref(found,'OutActors')))
        def check(enemy,index):
            g.branch(g.valid(g.call('GetController',PAWN,self=enemy)),lambda:g.set('Controllers',g.ie(g.get('Controllers'),1)))
            g.branch(g.gt(g.call('VSizeXY',M,A=g.call('GetVelocity',ACT,self=enemy)),5),lambda:g.set('Moving',g.ie(g.get('Moving'),1)))
        g.foreach(g.ref(found,'OutActors'),check)
        message='BLUEPRINT_ONLY_SMOKE'
        for label,name in [(' enemies=','EnemyCount'),(' controllers=','Controllers'),(' moving=','Moving')]:
            message=g.call('Concat_StrStr','/Script/Engine.KismetStringLibrary',A=message,B=g.call('Concat_StrStr','/Script/Engine.KismetStringLibrary',A=label,B=g.call('Conv_IntToString','/Script/Engine.KismetStringLibrary',InInt=g.get(name))))
        g.do('PrintString',K,InString=message,bPrintToScreen=False,bPrintToLog=True,Duration=1)
        g.do('QuitGame',K,SpecificPlayer=g.call('GetPlayerController',GS,PlayerIndex=0),QuitPreference='Quit',bIgnorePlatformRestrictions=True)
    g.branch(g.ge(g.get('Elapsed'),12),report)
g.branch(g.both(g.get('Enabled'),g.ne(g.get('Reported'))),observe)

def write_spec():
    path=Path(__file__).with_name('blueprints.json'); path.write_text(json.dumps(dict(assets=assets),indent=2),encoding='utf-8'); return path
if __name__=='__main__': print(write_spec())
