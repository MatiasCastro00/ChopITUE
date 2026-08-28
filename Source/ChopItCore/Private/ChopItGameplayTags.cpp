#include "ChopItGameplayTags.h"

namespace ChopItGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Run_Bootstrap, "State.Run.Bootstrap", "The run is being composed and validated.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cycle_Day, "State.Cycle.Day", "Safe harvesting phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cycle_Dusk, "State.Cycle.Dusk", "Transition and quota grace phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cycle_Night, "State.Cycle.Night", "Escalating voluntary-risk combat phase.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cycle_Elite, "State.Cycle.Elite", "Cycle-ending elite encounter.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Cycle_Resolution, "State.Cycle.Resolution", "Daily rewards and conditions are resolved.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Run_VictoryChoice, "State.Run.VictoryChoice", "The seventh day is complete and the player chooses an outcome.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Run_Infinite, "State.Run.Infinite", "The run continues beyond the seventh day.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Run_Death, "State.Run.Death", "The player has lost the active run.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Run_Finished, "State.Run.Finished", "The run result has been finalized.");
}
