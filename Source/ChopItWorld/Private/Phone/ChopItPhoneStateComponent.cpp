#include "Phone/ChopItPhoneStateComponent.h"

UChopItPhoneStateComponent::UChopItPhoneStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UChopItPhoneStateComponent::TryTransition(const EChopItPhoneState Next)
{
	bool bAllowed = false;
	switch (State)
	{
	case EChopItPhoneState::Available:
		bAllowed = Next == EChopItPhoneState::Ringing || Next == EChopItPhoneState::Answered;
		break;
	case EChopItPhoneState::Ringing:
		bAllowed = Next == EChopItPhoneState::Available || Next == EChopItPhoneState::Answered;
		break;
	case EChopItPhoneState::Answered:
		bAllowed = Next == EChopItPhoneState::Transforming;
		break;
	case EChopItPhoneState::Transforming:
		bAllowed = Next == EChopItPhoneState::Dialogue;
		break;
	case EChopItPhoneState::Dialogue:
		bAllowed = Next == EChopItPhoneState::Completed;
		break;
	case EChopItPhoneState::Completed:
		break;
	}
	if (!bAllowed) return false;
	const EChopItPhoneState Previous = State;
	State = Next;
	OnStateChanged.Broadcast(Previous, State);
	return true;
}
