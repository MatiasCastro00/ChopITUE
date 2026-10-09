#include "Rewards/ChopItChestRevealScene.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AChopItChestRevealScene::AChopItChestRevealScene()
{
	PrimaryActorTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LightMaterial(TEXT("/Game/ChopIt/Items/Chest/M_ChestReveal_Light.M_ChestReveal_Light"));
	LightMaterialAsset = LightMaterial.Object;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Stage")));
	Model = CreateDefaultSubobject<USceneComponent>(TEXT("ChestModel"));
	Model->SetupAttachment(RootComponent);
	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("RearHinge"));
	Hinge->SetupAttachment(Model);
	Hinge->SetRelativeLocation(FVector(0, 44, 72));
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCamera"));
	Capture->SetupAttachment(RootComponent);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// SceneColorHDR carries the scene's inverted translucency alpha. The UI
	// material inverts it back to foreground opacity before compositing.
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Capture->ShowFlags.SetPostProcessing(false);
	Capture->FOVAngle = 42.f;
	Capture->ShowFlags.SetAtmosphere(false);
	Capture->ShowFlags.SetFog(false);
	Capture->ShowFlags.SetMotionBlur(false);
	InnerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightInsideChest"));
	InnerLight->SetupAttachment(Model);
	InnerLight->SetRelativeLocation(FVector(0, 0, 65));
	InnerLight->SetAttenuationRadius(400.f);
	InnerLight->SetIntensity(2500.f);
	Sparks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RisingLightParticles"));
	Sparks->SetupAttachment(Model);
	Rays = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InteriorLightShafts"));
	Rays->SetupAttachment(Model);
	Finale = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LegendaryBurst"));
	Finale->SetupAttachment(Model);
	Finale->SetRelativeLocation(FVector(0, 0, 72));
	Finale->SetAutoActivate(false);
}

void AChopItChestRevealScene::InitializeScene()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Items/Chest/M_BossChest_Wood.M_BossChest_Wood"));
	UMaterialInterface* Gold = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/ChopIt/Items/Chest/M_BossChest_Gold.M_BossChest_Gold"));
	auto Part = [&](USceneComponent* Parent, FVector Position, FVector Size, UMaterialInterface* Material, FRotator Rotation = FRotator::ZeroRotator)
	{
		auto* Mesh = NewObject<UStaticMeshComponent>(this);
		AddInstanceComponent(Mesh);
		Mesh->SetupAttachment(Parent);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetMaterial(0, Material);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetVisibleInSceneCaptureOnly(true);
		Mesh->SetRelativeLocation(Position);
		Mesh->SetRelativeScale3D(Size / 100.f);
		Mesh->SetRelativeRotation(Rotation);
		Mesh->RegisterComponent();
	};
	// An actual hollow chest: floor, four walls, planks, ironwork and a hinged barrel lid.
	Part(Model, FVector(0,0,8), FVector(148,94,12), Wood);
	for (float Side : {-1.f, 1.f})
	{
		Part(Model, FVector(Side*70,0,40), FVector(12,94,64), Wood);
		for (int32 Plank=0; Plank<4; ++Plank)
			Part(Model, FVector(-54+Plank*36,Side*43,40), FVector(34,10,64), Wood);
		Part(Model, FVector(0,Side*49,15), FVector(150,6,9), Gold);
		Part(Model, FVector(0,Side*49,70), FVector(150,6,7), Gold);
		for (float Band : {-48.f,48.f})
		{
			Part(Model, FVector(Band,Side*50,42), FVector(10,5,57), Gold);
			for (float Z : {23.f,60.f}) Part(Model, FVector(Band,Side*54,Z), FVector(5,4,5), Gold);
		}
	}
	for (int32 Segment=0; Segment<9; ++Segment)
	{
		const float Angle = PI * (Segment + 0.5f) / 9.f;
		const FVector Position(0, -44.f + 47.f*FMath::Cos(Angle), 25.f*FMath::Sin(Angle));
		const FRotator Rotation(0,0,FMath::RadiansToDegrees(Angle)-90.f);
		Part(Hinge, Position, FVector(153,18,7), Wood, Rotation);
		for (float Band : {-48.f,48.f}) Part(Hinge, Position+FVector(Band,0,3), FVector(11,18,8), Gold, Rotation);
	}
	Part(Model, FVector(0,-53,55), FVector(20,8,25), Gold);
	Part(Hinge, FVector(0,-91,-4), FVector(15,7,18), Gold);
	Glow = UMaterialInstanceDynamic::Create(LightMaterialAsset ? LightMaterialAsset.Get() : Gold, this);
	for (UInstancedStaticMeshComponent* Mesh : {Sparks.Get(), Rays.Get()})
	{
		Mesh->SetStaticMesh(Mesh == Sparks ? Sphere : Cube);
		Mesh->SetMaterial(0, Glow);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetVisibleInSceneCaptureOnly(true);
	}
	for (int32 I=0; I<144; ++I) Sparks->AddInstance(FTransform(FVector::ZeroVector));
	for (int32 I=0; I<24; ++I) Rays->AddInstance(FTransform(FVector::ZeroVector));
	for (int32 I=0; I<2; ++I)
	{
		auto* Light = NewObject<UPointLightComponent>(this);
		AddInstanceComponent(Light);
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(I==0 ? FVector(-170,-190,230) : FVector(150,100,180));
		Light->SetIntensity(I==0 ? 13000.f : 9000.f);
		Light->SetAttenuationRadius(700.f);
		Light->SetLightColor(I==0 ? FLinearColor(1.f,.78f,.53f) : FLinearColor(.35f,.55f,1.f));
		Light->RegisterComponent();
	}
	Finale->SetAsset(LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/ChopIt/Items/Chest/NS_BossChest_Open.NS_BossChest_Open")));
	Finale->SetForceSolo(true);
	Finale->SetVisibleInSceneCaptureOnly(true);
	Finale->SetComponentTickEnabled(false);
	Finale->SetSystemFixedBounds(FBox(FVector(-400), FVector(400)));
	Target = NewObject<UTextureRenderTarget2D>(this);
	Target->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	Target->RenderTargetFormat = RTF_RGBA16f;
	Target->InitAutoFormat(1024,1024);
	Target->UpdateResourceImmediate(true);
	if (UMaterialInterface* CompositeMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/ChopIt/Items/Chest/M_ChestReveal_UI.M_ChestReveal_UI")))
	{
		CaptureCompositeMaterial = UMaterialInstanceDynamic::Create(CompositeMaterial, this);
		CaptureCompositeMaterial->SetTextureParameterValue(TEXT("CaptureTexture"), Target);
	}
	Capture->TextureTarget = Target;
	Capture->ShowOnlyActorComponents(this);
	UpdatePresentation(0.f, 3.f, 0);
}

void AChopItChestRevealScene::UpdatePresentation(float Time, float Duration, int32 Tier)
{
	if (!Target) return;
	Tier = FMath::Clamp(Tier,0,3);
	const int32 Stage = FMath::Clamp(FMath::FloorToInt((Time-.45f)/.82f),0,Tier);
	const FLinearColor Colors[] = {FLinearColor(.7f,.73f,.8f),FLinearColor(.12f,1.f,.3f),FLinearColor(.12f,.38f,1.f),FLinearColor(1.f,.55f,.06f)};
	const float Opening = FMath::SmoothStep(.2f,Duration,Time);
	const float Pulse = .5f+.5f*FMath::Sin(Time*(8.f+Stage*2.f));
	const float FinalePulse = FMath::Max(0.f, 1.f-FMath::Abs(Time-Duration)/.4f);
	Model->SetRelativeLocation(FVector(FMath::Sin(Time*39.f)*(Stage+1)*.45f*Opening,0,2.f*FMath::Sin(Time*3.f)));
	Model->SetRelativeRotation(FRotator(0, FMath::Sin(Time*1.7f)*(1.f+Stage), 0));
	Hinge->SetRelativeRotation(FRotator(0,0,105.f*Opening));
	Glow->SetVectorParameterValue(TEXT("ChestColor"), Colors[Stage]*(3.f+Pulse*2.f+FinalePulse*8.f));
	InnerLight->SetLightColor(Colors[Stage]);
	InnerLight->SetIntensity((250.f+Stage*250.f)*(Opening+.15f)*(1.f+Pulse*.3f+FinalePulse*2.f));
	const int32 Count = 16 + Stage*32;
	for (int32 I=0; I<144; ++I)
	{
		const float Age = FMath::Frac(Time*(.45f+Stage*.1f)+I*.618034f);
		const float Angle = I*2.39996f + Age*(Stage>=2 ? 7.f : 1.f);
		const float Radius = (8.f+Age*(18.f+Stage*14.f));
		// Every trajectory begins in the cavity and expands only after crossing the rim.
		const FVector P(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius*.65f,58.f+Age*(95.f+Stage*38.f));
		const float Size = I<Count ? Opening*(1.f-Age)*(.018f+Stage*.007f) : 0.f;
		Sparks->UpdateInstanceTransform(I,FTransform(FRotator::ZeroRotator,P,FVector(Size,Size,Size*(2.f+Stage))),false,false,true);
	}
	Sparks->MarkRenderStateDirty();
	for (int32 I=0; I<24; ++I)
	{
		const float Angle = I*2.39996f+Time*.15f;
		const FVector Direction = FVector(FMath::Cos(Angle)*.48f,FMath::Sin(Angle)*.32f,1.f).GetSafeNormal();
		const float Length = (75.f+Stage*23.f)*(Opening)*(1.f+.12f*FMath::Sin(Time*9.f+I));
		const FVector Origin(FMath::Cos(Angle)*12.f,FMath::Sin(Angle)*8.f,60.f);
		const float Width = I < 3+Stage*5 ? (.65f+Stage*.3f)*Opening : 0.f;
		Rays->UpdateInstanceTransform(I,FTransform(FRotationMatrix::MakeFromZ(Direction).ToQuat(),Origin+Direction*Length*.5f,FVector(Width/100.f,Width/100.f,Length/100.f)),false,false,true);
	}
	Rays->MarkRenderStateDirty();
	if (Tier==3 && Time>=Duration-.2f && !bBurst) { bBurst=true; Finale->Activate(true); Finale->SetComponentTickEnabled(false); }
	if (bBurst) Finale->AdvanceSimulationByTime(FMath::Clamp(Time-LastTime,0.f,.05f),1.f/60.f);
	LastTime=Time;
	const float Push = 1.f-.06f*Opening-.035f*FinalePulse;
	const FVector CameraPosition = FVector(240,-380,230)*Push;
	Capture->SetRelativeLocation(CameraPosition);
	Capture->SetRelativeRotation((FVector(0,0,100)-CameraPosition).Rotation());
	Capture->CaptureScene();
}
