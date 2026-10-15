#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage7Test, "ChopIt.Phone.Stage7.RotaryVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage7Test::RunTest(const FString& Parameters)
{
	UStaticMesh* Base = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Game/NewChopIt/Art/Models/Phone/SM_RotaryPhoneBase.SM_RotaryPhoneBase"));
	UStaticMesh* Receiver = LoadObject<UStaticMesh>(nullptr,
		TEXT("/Game/NewChopIt/Art/Models/Phone/SM_RotaryPhoneReceiver.SM_RotaryPhoneReceiver"));
	if (!TestNotNull(TEXT("Rotary base mesh"), Base) || !TestNotNull(TEXT("Rotary receiver mesh"), Receiver)) return false;
	TestTrue(TEXT("Base has dial and trim material slots"), Base->GetStaticMaterials().Num() >= 4);
	TestTrue(TEXT("Receiver has multiple detail materials"), Receiver->GetStaticMaterials().Num() >= 3);
	TestTrue(TEXT("Base has usable size"), Base->GetBounds().SphereRadius > 20.0f && Base->GetBounds().SphereRadius < 150.0f);
	TestTrue(TEXT("Receiver has usable size"), Receiver->GetBounds().SphereRadius > 15.0f && Receiver->GetBounds().SphereRadius < 120.0f);
	UChopItPhoneVariantData* Devil = LoadObject<UChopItPhoneVariantData>(nullptr,
		TEXT("/Game/NewChopIt/Art/Models/Phone/DA_PhoneDevil.DA_PhoneDevil"));
	UChopItPhoneVariantData* Angel = LoadObject<UChopItPhoneVariantData>(nullptr,
		TEXT("/Game/NewChopIt/Art/Models/Phone/DA_PhoneAngel.DA_PhoneAngel"));
	if (!TestNotNull(TEXT("Devil variant"), Devil) || !TestNotNull(TEXT("Angel variant"), Angel)) return false;
	TestTrue(TEXT("Both variants use the same configurable model"),
		Devil->BaseMesh.LoadSynchronous() == Base && Angel->BaseMesh.LoadSynchronous() == Base &&
		Devil->ReceiverMesh.LoadSynchronous() == Receiver && Angel->ReceiverMesh.LoadSynchronous() == Receiver);
	TestTrue(TEXT("Variants have different shell materials"),
		Devil->BaseMaterial.LoadSynchronous() && Angel->BaseMaterial.LoadSynchronous() &&
		Devil->BaseMaterial.LoadSynchronous() != Angel->BaseMaterial.LoadSynchronous());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Phone actor"), Phone)) return false;
	TestTrue(TEXT("Visible world phone uses rotary meshes before answering"),
		Phone->GetBaseVisual()->GetStaticMesh() == Base && Phone->GetReceiverVisual()->GetStaticMesh() == Receiver);
	TestTrue(TEXT("Imported meshes retain intended centimeter scale"),
		Phone->GetBaseVisual()->GetRelativeScale3D().Equals(FVector::OneVector) &&
		Phone->GetReceiverVisual()->GetRelativeScale3D().Equals(FVector::OneVector));
	return true;
}
#endif
