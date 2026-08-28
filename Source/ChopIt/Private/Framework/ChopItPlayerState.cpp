#include "Framework/ChopItPlayerState.h"

#include "Economy/ChopItEconomyComponent.h"
#include "Progression/ChopItExperienceComponent.h"
#include "Progression/ChopItUpgradeOfferComponent.h"
#include "Shop/ChopItShopComponent.h"
#include "Pacts/ChopItPactComponent.h"
#include "Pacts/ChopItPactDefinition.h"
#include "Cycle/ChopItCycleStateMachineComponent.h"
#include "GameFramework/GameStateBase.h"

AChopItPlayerState::AChopItPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;
	EconomyComponent = CreateDefaultSubobject<UChopItEconomyComponent>(TEXT("EconomyComponent"));
	ExperienceComponent = CreateDefaultSubobject<UChopItExperienceComponent>(TEXT("ExperienceComponent"));
	UpgradeOfferComponent = CreateDefaultSubobject<UChopItUpgradeOfferComponent>(TEXT("UpgradeOfferComponent"));
	ShopComponent = CreateDefaultSubobject<UChopItShopComponent>(TEXT("ShopComponent"));
	PactComponent = CreateDefaultSubobject<UChopItPactComponent>(TEXT("PactComponent"));
}
void AChopItPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (AGameStateBase* State=GetWorld()->GetGameState()) if (UChopItCycleStateMachineComponent* Cycle=State->FindComponentByClass<UChopItCycleStateMachineComponent>()) Cycle->OnPhaseChanged.AddUniqueDynamic(this,&AChopItPlayerState::HandlePhase);
}
void AChopItPlayerState::HandlePhase(EChopItCyclePhase NewPhase,EChopItCyclePhase PreviousPhase,int32)
{
	if(NewPhase!=EChopItCyclePhase::Dusk || PactComponent->HasActiveOffer()) return;
	TArray<UChopItPactDefinition*> Offers;
	for(const TCHAR* Name:{TEXT("Furia"),TEXT("Ritmo"),TEXT("Botas")}) Offers.Add(LoadObject<UChopItPactDefinition>(nullptr,*FString::Printf(TEXT("/Game/ChopIt/Pacts/DA_Pact_%s.DA_Pact_%s"),Name,Name)));
	PactComponent->OpenOffers(Offers);
}
