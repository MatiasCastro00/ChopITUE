#include "Phone/ChopItMysteryPhone.h"
#include "Phone/ChopItPhoneRevealComponent.h"
#include "Phone/ChopItPhoneStateComponent.h"
#include "Phone/ChopItPhoneVariantComponent.h"
#include "Phone/ChopItPhoneVariantData.h"
#include "Phone/ChopItPhoneVariantPool.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/DefaultPawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItPhoneStage5Test, "ChopIt.Phone.Stage5.VariantsAndReveal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChopItPhoneStage5Test::RunTest(const FString& Parameters)
{
	UChopItPhoneVariantPool* Pool = LoadObject<UChopItPhoneVariantPool>(nullptr,
		TEXT("/Game/NewChopIt/Art/Models/Phone/DA_PhoneVariants.DA_PhoneVariants"));
	if (!TestNotNull(TEXT("Variant pool Data Asset"), Pool)) return false;
	TestTrue(TEXT("Variant pool valid"), Pool->IsValidConfig());
	TestEqual(TEXT("Two initial variants"), Pool->Variants.Num(), 2);
	if (Pool->Variants.Num() != 2) return false;
	TestEqual(TEXT("Devil weight"), Pool->Variants[0]->Weight, 80.0f);
	TestEqual(TEXT("Angel weight"), Pool->Variants[1]->Weight, 20.0f);
	TestEqual(TEXT("Initial roll is devil"), Pool->SelectByRoll(0.0f)->VariantId, FName(TEXT("Devil")));
	TestEqual(TEXT("Below 80 selects devil"), Pool->SelectByRoll(79.9f)->VariantId, FName(TEXT("Devil")));
	TestEqual(TEXT("At 80 selects angel"), Pool->SelectByRoll(80.0f)->VariantId, FName(TEXT("Angel")));
	TestEqual(TEXT("Final roll selects angel"), Pool->SelectByRoll(100.0f)->VariantId, FName(TEXT("Angel")));
	FRandomStream First(734), Second(734);
	for (int32 Index = 0; Index < 16; ++Index)
		TestEqual(TEXT("Same seed repeats variant sequence"), Pool->Select(First), Pool->Select(Second));

	UChopItPhoneVariantPool* Extended = NewObject<UChopItPhoneVariantPool>();
	Extended->Variants = Pool->Variants;
	UChopItPhoneVariantData* Third = NewObject<UChopItPhoneVariantData>(Extended);
	Third->VariantId = TEXT("Third");
	Third->DisplayName = FText::FromString(TEXT("Third variant"));
	Third->Weight = 30.0f;
	Extended->Variants.Add(Third);
	TestTrue(TEXT("New asset can extend selection"), Extended->IsValidConfig());
	TestTrue(TEXT("Third variant selected without changing selector"), Extended->SelectByRoll(110.0f) == Third);
	Third->VariantId = TEXT("Angel");
	TestFalse(TEXT("Duplicate IDs rejected"), Extended->IsValidConfig());

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AChopItMysteryPhone* Phone = World->SpawnActor<AChopItMysteryPhone>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Phone"), Phone)) return false;
	Phone->GetPhoneVariant()->InitializeSelectionSeed(451);
	Phone->DispatchBeginPlay();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Pawn = World->SpawnActor<ADefaultPawn>(ADefaultPawn::StaticClass(), FVector(100, 0, 0),
		FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("Interactor"), Pawn)) return false;
	TestTrue(TEXT("Answer starts selection"), Phone->Interact_Implementation(Pawn));
	UChopItPhoneVariantData* Selected = Phone->GetPhoneVariant()->GetSelectedVariant();
	if (!TestNotNull(TEXT("Exactly one selected variant"), Selected)) return false;
	TestEqual(TEXT("Selection advances to transformation"), Phone->GetPhoneState()->GetState(), EChopItPhoneState::Transforming);
	TestTrue(TEXT("Reveal color matches selected asset"), Phone->GetPhoneReveal()->GetRevealLight() != nullptr &&
		Phone->GetPhoneReveal()->GetRevealLight()->GetLightColor().Equals(Selected->AccentColor, 0.01f));
	Phone->GetPhoneReveal()->CompleteRevealEarly();
	TestEqual(TEXT("Reveal advances to dialogue"), Phone->GetPhoneState()->GetState(), EChopItPhoneState::Dialogue);
	TestEqual(TEXT("Variant remains selected"), Phone->GetPhoneVariant()->GetSelectedVariant(), Selected);
	TestFalse(TEXT("Phone cannot be answered twice"), Phone->Interact_Implementation(Pawn));
	return true;
}
#endif
