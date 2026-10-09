#include "Items/ChopItMandrakeScream.h"
#include "Items/ChopItItemComponent.h"
#include "Items/ChopItItemDataAsset.h"
#include "Items/ChopItPassiveEffects.h"
#include "Combat/ChopItHealthComponent.h"
#include "Targeting/ChopItTargetingSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "TimerManager.h"
#include "Items/ChopItScreamDebuffComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChopItMandrakeTest, "ChopIt.Items.MandrakeScream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopItMandrakeTest::RunTest(const FString& Parameters)
{
	const UChopItItemDataAsset* Authored = LoadObject<UChopItItemDataAsset>(nullptr,
		TEXT("/Game/ChopIt/Items/DA_Item_MandrakeStick.DA_Item_MandrakeStick"));
	TestNotNull(TEXT("Mandrake Data Asset"),Authored);
	if (Authored)
	{
		TestTrue(TEXT("Mandrake has its icon"),!Authored->Icon.IsNull());
		TestTrue(TEXT("Mandrake can appear as loot"),Authored->SpawnWeight>0.f);
		TestTrue(TEXT("Mandrake effect is assigned"),Authored->Effects.ContainsByPredicate(
			[](const UChopItItemEffect* Effect){ return IsValid(Cast<UChopItMandrakeEffect>(Effect)); }));
	}
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto Spawn = [World](const FVector& Location)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		auto* Root = NewObject<USceneComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	AActor* Player = Spawn(FVector(1000,0,0));
	AActor* Boss = Spawn(FVector(100,0,0));
	AActor* Tree = Spawn(FVector(130,0,0));
	AActor* FarEnemy = Spawn(FVector(600,0,0));
	AActor* Victim = Spawn(FVector::ZeroVector);
	auto HealthOn = [](AActor* Actor, EChopItDamageTargetKind Kind)
	{
		auto* Health = NewObject<UChopItHealthComponent>(Actor);
		Health->TargetKind = Kind;
		Actor->AddInstanceComponent(Health);
		Health->RegisterComponent();
		return Health;
	};
	UChopItHealthComponent* BossHealth = HealthOn(Boss,EChopItDamageTargetKind::Enemy);
	UChopItHealthComponent* TreeHealth = HealthOn(Tree,EChopItDamageTargetKind::Tree);
	UChopItHealthComponent* FarHealth = HealthOn(FarEnemy,EChopItDamageTargetKind::Enemy);
	UChopItHealthComponent* VictimHealth = HealthOn(Victim,EChopItDamageTargetKind::Enemy);
	auto* Inventory = NewObject<UChopItItemComponent>(Player);
	Player->AddInstanceComponent(Inventory);
	Inventory->RegisterComponent();
	World->BeginPlay();
	for (AActor* Actor : {Player,Boss,Tree,FarEnemy,Victim}) if (!Actor->HasActorBegunPlay()) Actor->DispatchBeginPlay();
	World->GetTimerManager().Tick(0.f);
	TestEqual(TEXT("Boss location"),Boss->GetActorLocation().X,100.0);
	TestTrue(TEXT("Targets registered"),World->GetSubsystem<UChopItTargetingSubsystem>()->GetRegisteredTargetCount()>=3);
	auto* Item = NewObject<UChopItItemDataAsset>();
	Item->ItemId = TEXT("TestInfestation");
	auto* Effect = NewObject<UChopItInfestationEffect>(Item);
	Item->Effects.Add(Effect);
	TestTrue(TEXT("Player equips infection"),Inventory->AddItem(Item));
	auto* MandrakeItem = NewObject<UChopItItemDataAsset>();
	MandrakeItem->ItemId = TEXT("TestMandrake");
	auto* MandrakeEffect = NewObject<UChopItMandrakeEffect>(MandrakeItem);
	MandrakeEffect->BaseValue = 1.f; // Guaranteed proc verifies the death-event wiring.
	MandrakeItem->Effects.Add(MandrakeEffect);
	TestTrue(TEXT("Player equips mandrake"),Inventory->AddItem(MandrakeItem));
	FChopItDamageSpec KillingBlow;
	KillingBlow.BaseDamage = 100.f;
	VictimHealth->ApplyDamage(KillingBlow,Player);
	AChopItMandrakeScream* Mandrake = nullptr;
	int32 SpawnCount = 0;
	for (TActorIterator<AChopItMandrakeScream> It(World); It; ++It) { Mandrake=*It; ++SpawnCount; }
	TestEqual(TEXT("One mandrake spawns at the kill location"),SpawnCount,1);
	if (!Mandrake) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false; }
	TestTrue(TEXT("Mandrake starts at victim"),Mandrake->GetActorLocation().Equals(Victim->GetActorLocation()));
	++GFrameCounter;
	World->GetTimerManager().Tick(.26f);
	TestEqual(TEXT("Boss receives one quarter second of screaming damage"),BossHealth->GetCurrentHealth(),98.f);
	TestEqual(TEXT("Tree is excluded"),TreeHealth->GetCurrentHealth(),100.f);
	TestEqual(TEXT("Enemy outside radius is excluded"),FarHealth->GetCurrentHealth(),100.f);
	const auto Items = Inventory->GetItems();
	const UChopItInfestationEffect* Infection = nullptr;
	for (const FChopItOwnedItem& Owned : Items)
		for (const UChopItItemEffect* Active : Owned.Effects)
			if (const auto* Found = Cast<UChopItInfestationEffect>(Active)) Infection = Found;
	TestTrue(TEXT("Scream also builds infection on boss"),Infection && Infection->GetInfestation(BossHealth)>0.f);
	TestNotNull(TEXT("Scream applies confusion to boss"),Boss->FindComponentByClass<UChopItScreamDebuffComponent>());
	ACharacter* SlowTarget = World->SpawnActor<ACharacter>();
	SlowTarget->GetCharacterMovement()->MaxWalkSpeed = 400.f;
	auto* SlowHealth = HealthOn(SlowTarget,EChopItDamageTargetKind::Enemy);
	if (!SlowTarget->HasActorBegunPlay()) SlowTarget->DispatchBeginPlay();
	auto* Slow = NewObject<UChopItScreamDebuffComponent>(SlowTarget);
	SlowTarget->AddInstanceComponent(Slow);Slow->RegisterComponent();
	Slow->Refresh(.65f,1.f);
	TestEqual(TEXT("Scream slows movement 35 percent"),SlowTarget->GetCharacterMovement()->MaxWalkSpeed,260.f);
	Slow->Refresh(.65f,1.f);
	TestEqual(TEXT("Repeated pulses do not multiply slow"),SlowTarget->GetCharacterMovement()->MaxWalkSpeed,260.f);
	Mandrake->Destroy();
	++GFrameCounter;World->GetTimerManager().Tick(1.1f);
	TestEqual(TEXT("Movement recovers after leaving scream"),SlowTarget->GetCharacterMovement()->MaxWalkSpeed,400.f);
	TestFalse(TEXT("Confusion expires"),Slow->IsSlowed());
	Slow->Refresh(.65f,1.f);
	SlowHealth->ApplyDamage(KillingBlow,Player);
	TestFalse(TEXT("Death clears confusion"),Slow->IsSlowed());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
