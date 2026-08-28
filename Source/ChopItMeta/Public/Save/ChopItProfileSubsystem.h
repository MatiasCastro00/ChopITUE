#pragma once
#include "Subsystems\GameInstanceSubsystem.h"
#include "ChopItProfileSubsystem.generated.h"
class UChopItProfileSaveGame;
UCLASS()
class CHOPITMETA_API UChopItProfileSubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void CommitRun(int64 EarnedMoney,bool bVictory);
	int64 GetBankedMoney() const;
private:
	void SaveProfile();
	UPROPERTY() TObjectPtr<UChopItProfileSaveGame> Profile;
	static constexpr const TCHAR* Slot=TEXT("ChopIt_Profile");
};
