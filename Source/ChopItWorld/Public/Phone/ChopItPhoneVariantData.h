#pragma once

#include "Engine/DataAsset.h"
#include "ChopItPhoneVariantData.generated.h"

class UStaticMesh;
class UMaterialInterface;
class USoundBase;
class UNiagaraSystem;
class UChopItDialogueSequence;

/** One weighted phone identity and its reveal presentation. */
UCLASS(BlueprintType)
class CHOPITWORLD_API UChopItPhoneVariantData : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	FName VariantId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection", meta=(ClampMin="0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reveal")
	FLinearColor AccentColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reveal", meta=(ClampMin="0", Units="s"))
	float RevealDuration = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reveal", meta=(ClampMin="0"))
	float PeakLightIntensity = 12000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reveal", meta=(ClampMin="0"))
	float IdleLightIntensity = 800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Reveal", meta=(ClampMin="1", Units="cm"))
	float LightRadius = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TSoftObjectPtr<UStaticMesh> BaseMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TSoftObjectPtr<UStaticMesh> ReceiverMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TSoftObjectPtr<UMaterialInterface> BaseMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TSoftObjectPtr<UMaterialInterface> ReceiverMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visuals")
	TSoftObjectPtr<UNiagaraSystem> RevealVFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio")
	TSoftObjectPtr<USoundBase> RevealSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue", meta=(ToolTip="Editable conversation shown after this variant is revealed."))
	TSoftObjectPtr<UChopItDialogueSequence> DialogueSequence;

	bool IsValidConfig() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
