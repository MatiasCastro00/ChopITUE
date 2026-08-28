#include "Save/ChopItProfileSubsystem.h"
#include "Save/ChopItProfileSaveGame.h"
#include "Kismet/GameplayStatics.h"
void UChopItProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection); Profile=Cast<UChopItProfileSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
	if(!Profile) Profile=Cast<UChopItProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UChopItProfileSaveGame::StaticClass()));
	Profile->Version=FMath::Max(1,Profile->Version);
}
void UChopItProfileSubsystem::CommitRun(int64 EarnedMoney,bool bVictory)
{
	const int64 Reward=FMath::Max<int64>(0,EarnedMoney)*(bVictory?75:25)/100; Profile->BankedMoney+=Reward; bVictory?++Profile->CompletedRuns:++Profile->FailedRuns; SaveProfile();
}
int64 UChopItProfileSubsystem::GetBankedMoney() const{return Profile?Profile->BankedMoney:0;}
void UChopItProfileSubsystem::SaveProfile(){if(Profile) UGameplayStatics::SaveGameToSlot(Profile,Slot,0);}
