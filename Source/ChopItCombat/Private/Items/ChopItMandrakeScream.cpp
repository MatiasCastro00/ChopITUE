#include "Items/ChopItMandrakeScream.h"

#include "Combat/ChopItHealthComponent.h"
#include "Targeting/ChopItTargetingSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AChopItMandrakeScream::AChopItMandrakeScream()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MandrakeRoot"));
	Body->SetupAttachment(RootComponent);
	Body->SetRelativeLocation(FVector(0,0,30));
	Body->SetRelativeScale3D(FVector(.32f,.27f,.43f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Leaves = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Leaves"));
	Leaves->SetupAttachment(RootComponent);
	Leaves->SetRelativeLocation(FVector(0,0,71));
	Leaves->SetRelativeScale3D(FVector(.34f,.34f,.35f));
	Leaves->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftEye"));
	RightEye = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightEye"));
	Mouth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreamingMouth"));
	for (UStaticMeshComponent* FacePart : {LeftEye.Get(),RightEye.Get(),Mouth.Get()})
	{
		FacePart->SetupAttachment(RootComponent);
		FacePart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FacePart->SetCastShadow(false);
	}
	LeftEye->SetRelativeLocation(FVector(29,-12,43));
	RightEye->SetRelativeLocation(FVector(29,12,43));
	LeftEye->SetRelativeScale3D(FVector(.045f));
	RightEye->SetRelativeScale3D(FVector(.045f));
	Mouth->SetRelativeLocation(FVector(31,0,24));
	Mouth->SetRelativeScale3D(FVector(.035f,.095f,.11f));
	SoundRings = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ScreamWaves"));
	SoundRings->SetupAttachment(RootComponent);
	SoundRings->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SoundRings->SetCastShadow(false);
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("ScreamLight"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetRelativeLocation(FVector(0,0,55));
	Glow->SetLightColor(FLinearColor(.4f,1.f,.25f));
	Glow->SetIntensity(1700.f);
	Glow->SetAttenuationRadius(280.f);
	Scream = CreateDefaultSubobject<UAudioComponent>(TEXT("ScreamAudio"));
	Scream->SetupAttachment(RootComponent);
	Scream->bAutoActivate = false;
	Scream->bOverrideAttenuation = true;
	Scream->AttenuationOverrides.bSpatialize = true;
	Scream->AttenuationOverrides.bAttenuate = true;
	Scream->AttenuationOverrides.FalloffDistance = 850.f;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RootMaterial(TEXT("/Game/ChopIt/Items/Mandrake/M_Mandrake_Root.M_Mandrake_Root"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LeafMaterial(TEXT("/Game/ChopIt/Items/Mandrake/M_Mandrake_Leaf.M_Mandrake_Leaf"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaveMaterial(TEXT("/Game/ChopIt/Items/Mandrake/M_Mandrake_Wave.M_Mandrake_Wave"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FaceMaterial(TEXT("/Game/ChopIt/Items/Mandrake/M_Mandrake_Face.M_Mandrake_Face"));
	static ConstructorHelpers::FObjectFinder<USoundWave> ScreamSound(TEXT("/Game/ChopIt/Items/Mandrake/S_Mandrake_Scream.S_Mandrake_Scream"));
	if (Sphere.Succeeded())
	{
		Body->SetStaticMesh(Sphere.Object); SoundRings->SetStaticMesh(Sphere.Object);
		LeftEye->SetStaticMesh(Sphere.Object); RightEye->SetStaticMesh(Sphere.Object); Mouth->SetStaticMesh(Sphere.Object);
	}
	if (Cone.Succeeded()) Leaves->SetStaticMesh(Cone.Object);
	if (RootMaterial.Succeeded()) Body->SetMaterial(0,RootMaterial.Object);
	if (LeafMaterial.Succeeded()) Leaves->SetMaterial(0,LeafMaterial.Object);
	if (WaveMaterial.Succeeded()) SoundRings->SetMaterial(0,WaveMaterial.Object);
	if (FaceMaterial.Succeeded())
	{
		LeftEye->SetMaterial(0,FaceMaterial.Object);
		RightEye->SetMaterial(0,FaceMaterial.Object);
		Mouth->SetMaterial(0,FaceMaterial.Object);
	}
	if (ScreamSound.Succeeded()) Scream->SetSound(ScreamSound.Object);
}

void AChopItMandrakeScream::Initialize(AActor* InOwner, float InRadius, float InDamagePerSecond, float InDuration)
{
	DamageOwner = InOwner;
	Radius = FMath::Max(1.f, InRadius);
	DamagePerSecond = FMath::Max(0.f, InDamagePerSecond);
	SetLifeSpan(FMath::Max(.1f, InDuration));
	for (int32 I=0; I<36; ++I) SoundRings->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
	Scream->Play();
	GetWorldTimerManager().SetTimer(DamageTimer,this,&ThisClass::DamagePulse,.25f,true,.25f);
}

void AChopItMandrakeScream::DamagePulse()
{
	if (!DamageOwner.IsValid() || !GetWorld()) return;
	UChopItTargetingSubsystem* Targets = GetWorld()->GetSubsystem<UChopItTargetingSubsystem>();
	if (!Targets) return;
	for (UChopItHealthComponent* Health : Targets->FindTargetsInRadius(GetActorLocation(),Radius,MAX_int32,DamageOwner.Get()))
	{
		if (!IsValid(Health) || Health->TargetKind != EChopItDamageTargetKind::Enemy) continue;
		FChopItDamageSpec Damage;
		Damage.BaseDamage = DamagePerSecond*.25f;
		Health->ApplyDamage(Damage,DamageOwner.Get(),Health->GetOwner()->GetActorLocation());
	}
}

void AChopItMandrakeScream::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float Beat = .5f+.5f*FMath::Sin(Age*PI*8.f);
	Body->SetRelativeScale3D(FVector(.32f,.27f,.43f)*(1.f+Beat*.14f));
	Leaves->SetRelativeRotation(FRotator(0,Age*42.f,FMath::Sin(Age*8.f)*12.f));
	Mouth->SetRelativeScale3D(FVector(.035f,.095f,.11f)*(1.f+Beat*.28f));
	Glow->SetIntensity(800.f+Beat*2200.f);
	for (int32 I=0; I<36; ++I)
	{
		const int32 Ring = I/12;
		const float Phase = FMath::Frac(Age*1.7f+Ring/3.f);
		const float Angle = (I%12)*(2.f*PI/12.f);
		const float Distance = Phase*Radius;
		const FVector Position(FMath::Cos(Angle)*Distance,FMath::Sin(Angle)*Distance,55.f+Phase*28.f);
		const float Size = .12f*(1.f-Phase);
		SoundRings->UpdateInstanceTransform(I,FTransform(FRotator::ZeroRotator,Position,FVector(Size)),false,false,true);
	}
	SoundRings->MarkRenderStateDirty();
}
