#include "Items/ChopItMandrakeScream.h"
#include "Items/ChopItScreamDebuffComponent.h"
#include "Combat/ChopItHealthComponent.h"
#include "Targeting/ChopItTargetingSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AChopItMandrakeScream::AChopItMandrakeScream()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MandrakeRoot"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeRotation(FRotator(0,-90,0));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Model(TEXT("/Game/ChopIt/Items/Mandrake/SM_Mandrake.SM_Mandrake"));
	if (Model.Succeeded())
	{
		Body->SetStaticMesh(Model.Object);
		ModelScale = 105.f/FMath::Max(.001f,Model.Object->GetBounds().BoxExtent.Z*2.f);
		Body->SetRelativeScale3D(FVector(ModelScale));
		Body->SetRelativeLocation(FVector(0,0,-(Model.Object->GetBounds().Origin.Z-Model.Object->GetBounds().BoxExtent.Z)*ModelScale));
	}
	SoundWaves = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SonicWaves"));
	LeftSplash = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LeftTearSplash"));
	RightSplash = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RightTearSplash"));
	Emergence = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EmergenceMotes"));
	for (UNiagaraComponent* FX : {SoundWaves.Get(),LeftSplash.Get(),RightSplash.Get(),Emergence.Get()})
	{
		FX->SetupAttachment(RootComponent);
		FX->SetAutoActivate(false);
	}
	SoundWaves->SetRelativeLocation(FVector(18,0,43));
	LeftSplash->SetRelativeLocation(FVector(23,21,3));
	RightSplash->SetRelativeLocation(FVector(23,-30,3));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Waves(TEXT("/Game/ChopIt/Items/Mandrake/NS_Mandrake_SonicWaves.NS_Mandrake_SonicWaves"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Splash(TEXT("/Game/ChopIt/Items/Mandrake/NS_Mandrake_TearSplash.NS_Mandrake_TearSplash"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Emerge(TEXT("/Game/ChopIt/Items/Mandrake/NS_Mandrake_Emerge.NS_Mandrake_Emerge"));
	SoundWaves->SetAsset(Waves.Object);
	LeftSplash->SetAsset(Splash.Object);RightSplash->SetAsset(Splash.Object);
	Emergence->SetAsset(Emerge.Object);
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("ScreamLight"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetRelativeLocation(FVector(30,0,55));
	Glow->SetLightColor(FLinearColor(.25f,.65f,1.f));
	Glow->SetIntensity(250.f);
	Glow->SetCastShadows(false);
	Glow->SetAttenuationRadius(180.f);
	Scream = CreateDefaultSubobject<UAudioComponent>(TEXT("ScreamAudio"));
	Scream->SetupAttachment(RootComponent);
	Scream->bAutoActivate = false;
	Scream->bOverrideAttenuation = true;
	Scream->AttenuationOverrides.bSpatialize = true;
	Scream->AttenuationOverrides.bAttenuate = true;
	Scream->AttenuationOverrides.FalloffDistance = 850.f;
	static ConstructorHelpers::FObjectFinder<USoundWave> Sound(TEXT("/Game/ChopIt/Items/Mandrake/S_Mandrake_Scream.S_Mandrake_Scream"));
	Scream->SetSound(Sound.Object);
}

void AChopItMandrakeScream::Initialize(AActor* InOwner,float InRadius,float InDPS,float InDuration,float InSlowMultiplier,float InSlowDuration)
{
	DamageOwner = InOwner;
	Radius = FMath::Max(1.f,InRadius);
	DamagePerSecond = FMath::Max(0.f,InDPS);
	Lifetime = FMath::Max(.1f,InDuration);
	SlowMultiplier = FMath::Clamp(InSlowMultiplier,.1f,1.f);
	SlowDuration = FMath::Max(.05f,InSlowDuration);
	SetLifeSpan(Lifetime);
	Body->PrestreamTextures(Lifetime,true);
	SoundWaves->SetRelativeScale3D(FVector(Radius/350.f,Radius/350.f,1));
	for (UNiagaraComponent* FX : {SoundWaves.Get(),LeftSplash.Get(),RightSplash.Get(),Emergence.Get()}) FX->Activate(true);
	Scream->Play();
	GetWorldTimerManager().SetTimer(DamageTimer,this,&ThisClass::DamagePulse,.25f,true,.25f);
}

void AChopItMandrakeScream::StartVisualPreview(float PreviewDuration)
{
	DamageOwner.Reset();
	DamagePerSecond = 0.f;
	Age = 0.f;
	Lifetime = FMath::Max(.1f,PreviewDuration);
	Body->PrestreamTextures(Lifetime,true);
	SoundWaves->SetRelativeScale3D(FVector(1.f,1.f,1.f));
	for (UNiagaraComponent* FX : {SoundWaves.Get(),LeftSplash.Get(),RightSplash.Get(),Emergence.Get()}) FX->Activate(true);
	Scream->Play();
}

void AChopItMandrakeScream::DamagePulse()
{
	if (!DamageOwner.IsValid() || !GetWorld()) return;
	UChopItTargetingSubsystem* Targets = GetWorld()->GetSubsystem<UChopItTargetingSubsystem>();
	if (!Targets) return;
	for (UChopItHealthComponent* Health : Targets->FindTargetsInRadius(GetActorLocation(),Radius,MAX_int32,DamageOwner.Get()))
	{
		if (!IsValid(Health) || Health->TargetKind != EChopItDamageTargetKind::Enemy) continue;
		FChopItDamageSpec Damage;Damage.BaseDamage = DamagePerSecond*.25f;
		const float Applied = Health->ApplyDamage(Damage,DamageOwner.Get(),Health->GetOwner()->GetActorLocation());
		if (Applied>0.f && IsValid(Health->GetOwner()) && Health->IsAlive())
		{
			AActor* Target = Health->GetOwner();
			UChopItScreamDebuffComponent* Debuff = Target->FindComponentByClass<UChopItScreamDebuffComponent>();
			if (!Debuff)
			{
				Debuff = NewObject<UChopItScreamDebuffComponent>(Target);
				Target->AddInstanceComponent(Debuff);Debuff->RegisterComponent();
			}
			Debuff->Refresh(SlowMultiplier,SlowDuration);
		}
	}
}

void AChopItMandrakeScream::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;
	const float Beat = .5f+.5f*FMath::Sin(Age*PI*8.f);
	const float Envelope = FMath::Clamp(FMath::Min(Age/.2f,(Lifetime-Age)/.3f),.01f,1.f);
	Body->SetRelativeScale3D(FVector(1.f+Beat*.025f,1.f+Beat*.025f,1.f-Beat*.018f)*ModelScale*Envelope);
	Glow->SetIntensity((160.f+Beat*230.f)*Envelope);
}
