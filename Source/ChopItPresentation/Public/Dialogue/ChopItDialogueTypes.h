#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Camera/ChopItCameraTypes.h"
#include "ChopItDialogueTypes.generated.h"

class AActor;
class UChopItDialogueSequence;

UENUM(BlueprintType)
enum class EChopItDialogueStartPolicy : uint8
{
	Queue,
	Replace,
	RejectIfBusy
};

UENUM(BlueprintType)
enum class EChopItDialogueState : uint8
{
	Closed,
	Entering,
	Revealing,
	AwaitingAdvance,
	ShowingChoices,
	Exiting
};

UENUM(BlueprintType)
enum class EChopItDialogueEndReason : uint8
{
	Completed,
	Cancelled,
	Replaced,
	Stopped,
	InvalidData,
	VisitLimit
};

UENUM(BlueprintType)
enum class EChopItDialoguePortraitSide : uint8
{
	Left,
	Right
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueHandle
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dialogue")
	FGuid Id;

	bool IsValid() const { return Id.IsValid(); }
	void Invalidate() { Id.Invalidate(); }
	friend bool operator==(const FChopItDialogueHandle& A, const FChopItDialogueHandle& B) { return A.Id == B.Id; }
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueArgument
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FText Value;
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TObjectPtr<AActor> Actor;
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") FGameplayTagContainer Tags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FChopItDialogueArgument> Arguments;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dialogue") TArray<FChopItDialogueBinding> Bindings;

	AActor* ResolveBinding(FName Name) const
	{
		for (const FChopItDialogueBinding& Binding : Bindings)
		{
			if (Binding.Name == Name) return Binding.Actor;
		}
		return nullptr;
	}
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueEventPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FChopItDialogueHandle Handle;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") TObjectPtr<const UChopItDialogueSequence> Sequence;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FName LineId;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FName SpeakerId;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FName MarkerId;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FName ChoiceId;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FName TargetBinding;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") TObjectPtr<AActor> Target;
};

USTRUCT(BlueprintType)
struct CHOPITPRESENTATION_API FChopItDialogueChoiceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FName ChoiceId;
	UPROPERTY(BlueprintReadOnly, Category="Dialogue") FText Text;
};

