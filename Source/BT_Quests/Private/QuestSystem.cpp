// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "QuestSystem.h"

#if AsyncMessageSystem_Enabled
#include "AsyncMessageSystemBase.h"
#include "AsyncMessageWorldSubsystem.h"
#endif
#include "BT_Quests.h"
#include "DataAssets/QuestChain.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Kismet/GameplayStatics.h"
#include "Objects/QuestRequirementBase.h"

#if TAGFACTS_INSTALLED
#include "Core/FactSubSystem.h"
#endif

UQuestSystem::UQuestSystem()
{
}

UQuestSystem* UQuestSystem::Get()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetQuestSystem)
	return GEngine->GameViewport->GetWorld()->GetGameInstance()->GetSubsystem<UQuestSystem>();
}

bool UQuestSystem::AcceptQuest(TSoftObjectPtr<UQuestAsset> Quest, bool ForceAccept)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AcceptQuest)
	
	if(Quest.IsNull())
	{
		return false;
	}
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}
	
	if(!CanAcceptQuest(Quest) && !ForceAccept)
	{
		return false;
	}

	//Player can accept the quest, start accepting it.

	//Wrap the quest into a struct that is more easily
	//serialized and manageable.
	FBTQuestWrapper QuestWrapper = CreateQuestWrapper(Quest);

	QuestSubSystem->Quests.Add(Quest, QuestWrapper);
	for(auto& CurrentChain : Quest->QuestChains)
	{
		if(!QuestSubSystem->QuestChains.Contains(CurrentChain))
		{
			QuestSubSystem->QuestChains.Add(CurrentChain);
			PRAGMA_DISABLE_DEPRECATION_WARNINGS; 
			QuestSubSystem->QuestChainStarted.Broadcast(CurrentChain);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS;
		}
	}

	QuestSubSystem->QuestAccepted.Broadcast(QuestWrapper);

	#if ENABLE_VISUAL_LOG
	{
		/**Log the location and time of the player when the quest is accepted*/
		UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
			10, FColor::White, TEXT("Accepted Quest: %s"), *Quest.GetAssetName());
	}
	#endif

	#if AsyncMessageSystem_Enabled
	if(TSharedPtr<FAsyncMessageSystemBase> Sys = UAsyncMessageWorldSubsystem::GetSharedMessageSystem(QuestSubSystem->GetWorld()))
	{
		Sys->QueueMessageForBroadcast(
			FAsyncMessageId(Quest.LoadSynchronous()->QuestID), 
			FInstancedStruct::Make(QuestWrapper));
	}
	#endif

	UE_LOG(LogQuestSystem, Log, TEXT("Accepted Quest %s"), *Quest.GetAssetName());
	
	return true;
}

bool UQuestSystem::CanAcceptQuest(TSoftObjectPtr<UQuestAsset> Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CanAcceptQuest)
	
	if(GetQuestState(Quest) != EBTQuestState::Inactive)
	{
		UE_LOG(LogQuestSystem, Log, TEXT("Can't accept quest %s, as it's not inactive."), *Quest.GetAssetName());
		return false;
	}

	for(auto& CurrentRequirement : Quest.LoadSynchronous()->Requirements)
	{
		if(!CurrentRequirement->IsConditionMet(Quest))
		{
			UE_LOG(LogQuestSystem, Log, TEXT("Can't accept quest %s, failed requirement %s"), *Quest.GetAssetName(), *CurrentRequirement->GetName());
			return false;
		}
	}

	if(!HasCompletedRequiredQuests(Quest))
	{
		UE_LOG(LogQuestSystem, Log, TEXT("Can't accept quest %s, not completed required quests"), *Quest.GetAssetName());
		return false;
	}

	return true;
}

void UQuestSystem::CompleteQuest(TSoftObjectPtr<UQuestAsset> Quest, bool SkipCompletionCheck, bool AutoAcceptQuest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CompleteQuest)
	
	if(Quest.IsNull())
	{
		return;
	}
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return;
	}
	
	FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(Quest);
	if(!QuestWrapper)
	{
		if(AutoAcceptQuest)
		{
			AcceptQuest(Quest, true);
			QuestWrapper = QuestSubSystem->Quests.Find(Quest);
		}
		else
		{
			return;
		}
	}
	
	if(!SkipCompletionCheck)
	{
		if(!QuestSubSystem->CanCompleteQuest(*QuestWrapper))
		{
			return;
		}
	}

	QuestWrapper->State = EBTQuestState::Completed;

	//Safety check, mostly happens when a quest is force completed through a dev tool.
	for(auto& CurrentQuest : GetRequiredQuestsForQuest(Quest))
	{
		if(GetQuestState(CurrentQuest) != EBTQuestState::Completed && CurrentQuest != Quest)
		{
			CompleteQuest(CurrentQuest, true);
		}
	}

	/**If we are forcing this quest completion through the editor/dev tools,
	 * then we need to forcibly complete non-optional objectives as well.*/
	for(auto& CurrentStage : QuestWrapper->ObjectiveStages)
	{
		for(auto& CurrentObjective : CurrentStage.Objectives)
		{
			if(!CurrentObjective.IsOptional && CurrentObjective.State == EBTQuestState::InProgress)
			{
				CompleteObjective(CurrentObjective.ObjectiveID, nullptr);
			}
			// else
			// {
			// 	FailObjective(CurrentObjective.ObjectiveID, false);
			// }
		}
	}
	
	QuestSubSystem->QuestCompleted.Broadcast(*QuestWrapper);

	#if TAGFACTS_INSTALLED
	/**If TagFacts is installed, we increment a fact by one.
	 * This fact matches the Quest ID, so we can track if
	 * this quest was completed.*/
	UFactSubSystem::Get()->IncrementFact(Quest.LoadSynchronous()->QuestID);
	#endif
	
	#if AsyncMessageSystem_Enabled
	if(TSharedPtr<FAsyncMessageSystemBase> Sys = UAsyncMessageWorldSubsystem::GetSharedMessageSystem(QuestSubSystem->GetWorld()))
	{
		Sys->QueueMessageForBroadcast(
			FAsyncMessageId(Quest.Get()->QuestID), 
			FInstancedStruct::Make(*QuestWrapper));
	}
	#endif
		
	#if ENABLE_VISUAL_LOG
	UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
	10, FColor::White, TEXT("Completed quest: %s"), *Quest->GetName());
	#endif
}

bool UQuestSystem::CanCompleteQuest(TSoftObjectPtr<UQuestAsset> Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CanCompleteQuestSlow)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}

	if(FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(Quest))
	{
		return QuestSubSystem->CanCompleteQuest(*QuestWrapper);
	}

	return false;
}

bool UQuestSystem::CanCompleteQuest(FBTQuestWrapper Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CanCompleteQuest)
	
	if(Quest.State != EBTQuestState::InProgress)
	{
		UE_LOG(LogQuestSystem, Log, TEXT("Tried to complete quest %s, but it's not in progress."), *Quest.QuestAsset.GetAssetName());
		return false;
	}

	if(Quest.ObjectiveStages.IsEmpty())
	{
		UE_LOG(LogQuestSystem, Log, TEXT("Quest %s has no objective stages, can't complete."), *Quest.QuestAsset.GetAssetName());
		return false;
	}

	for(auto& CurrentStage : Quest.ObjectiveStages)
	{
		if(!CurrentStage.IsComplete())
		{
			UE_LOG(LogQuestSystem, Log, TEXT("Tried to complete quest %s, but an objective is still in progress."), *Quest.QuestAsset.GetAssetName());
			return false;
		}
	}

	return true;
}

EBTQuestState UQuestSystem::GetQuestState(TSoftObjectPtr<UQuestAsset> Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetQuestState)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return EBTQuestState::Inactive;
	}
	
	if(FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(Quest))
	{
		return QuestWrapper->State;
	}

	return EBTQuestState::Inactive;
}

bool UQuestSystem::AbandonQuest(TSoftObjectPtr<UQuestAsset> Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(AbandonQuest)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}
	
	FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(Quest);
	if(!QuestWrapper)
	{
		return false;
	}

	QuestSubSystem->QuestAbandoned.Broadcast(*QuestWrapper);

	QuestSubSystem->Quests.Remove(Quest);

	#if ENABLE_VISUAL_LOG
	{
		UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
		10, FColor::White, TEXT("Abandoned quest: %s"), *Quest->GetName());
	}
	#endif

	UE_LOG(LogQuestSystem, Log, TEXT("Abandoned quest %s"), *Quest.GetAssetName());

	return true;
}

bool UQuestSystem::FailQuest(TSoftObjectPtr<UQuestAsset> Quest, bool FailObjectives)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FailQuest)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}
	
	FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(Quest);
	if(!QuestWrapper)
	{
		return false;
	}
	
	if(QuestWrapper->State != EBTQuestState::InProgress)
	{
		return false;
	}

	QuestWrapper->State = EBTQuestState::Failed;

	#if ENABLE_VISUAL_LOG
	{
		UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
		10, FColor::White, TEXT("Failed quest: %s"),
		*Quest->GetName());
	}
	#endif
		
	if(FailObjectives)
	{
		//Fail the objectives
		for(auto& CurrentObjective : QuestWrapper->ObjectiveStages[QuestWrapper->CurrentStage].Objectives)
		{
			CurrentObjective.State = EBTQuestState::Failed;
			QuestSubSystem->ObjectiveFailed.Broadcast(CurrentObjective);

			#if AsyncMessageSystem_Enabled
			if(TSharedPtr<FAsyncMessageSystemBase> Sys = UAsyncMessageWorldSubsystem::GetSharedMessageSystem(QuestSubSystem->GetWorld()))
			{
				Sys->QueueMessageForBroadcast(
					FAsyncMessageId(Quest.LoadSynchronous()->QuestID), 
					FInstancedStruct::Make(CurrentObjective));
			}
			#endif
		}
	}

	QuestSubSystem->QuestFailed.Broadcast(*QuestWrapper);
	
	#if AsyncMessageSystem_Enabled
	if(TSharedPtr<FAsyncMessageSystemBase> Sys = UAsyncMessageWorldSubsystem::GetSharedMessageSystem(QuestSubSystem->GetWorld()))
	{
		Sys->QueueMessageForBroadcast(
			FAsyncMessageId(Quest.Get()->QuestID), 
			FInstancedStruct::Make(*QuestWrapper));
	}
	#endif

	return true;
}

TArray<FBTQuestWrapper> UQuestSystem::GetQuestsWithState(EBTQuestState State)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetQuestsWithState)
	
	TArray<FBTQuestWrapper> FoundQuests;

	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return FoundQuests;
	}

	for(auto& CurrentQuest : QuestSubSystem->Quests)
	{
		if(CurrentQuest.Value.State == State)
		{
			FoundQuests.Add(CurrentQuest.Value);
		}
	}

	return FoundQuests;
}

TArray<TSoftObjectPtr<UQuestAsset>> UQuestSystem::GetRequiredQuestsForQuest(TSoftObjectPtr<UQuestAsset> Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetRequiredQuestsForQuest)
	
	TArray<TSoftObjectPtr<UQuestAsset>> Quests;
	UQuestAsset* LoadedQuest = Quest.LoadSynchronous();

	if(!LoadedQuest->QuestChains.IsValidIndex(0))
	{
		return Quests;
	}
	
	//Go through all chains and their stages
	for(auto& CurrentChain : LoadedQuest->QuestChains)
	{
		CurrentChain.LoadSynchronous();
		//Find the stage containing this quest
		for(int32 FoundStage = 0; FoundStage < CurrentChain->Stages.Num(); FoundStage++)
		{
			if(CurrentChain->Stages[FoundStage].Quests.Contains(Quest))
			{
				//Quest is part of the root stage required to start this chain.
				//It has no required quests in this chain.
				if(FoundStage == 0)
				{
					break;
				}
				
				//Reverse loop to the previous stages.
				for(int32 ScannedStages = FoundStage - 1; ScannedStages >= 0; ScannedStages--)
				{
					for(auto& CurrentQuest : CurrentChain->Stages[ScannedStages].Quests)
					{
						Quests.AddUnique(CurrentQuest);
					}
				}
			}
		}
	}
	
	return Quests;
}

bool UQuestSystem::HasCompletedRequiredQuests(TSoftObjectPtr<UQuestAsset> Quest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(HasCompletedRequiredQuests)
	
	UQuestAsset* LoadedQuest = Quest.LoadSynchronous();

	if(!LoadedQuest->QuestChains.IsValidIndex(0))
	{
		return true;
	}
	
	//Go through all chains and their stages
	for(auto& CurrentChain : LoadedQuest->QuestChains)
	{
		CurrentChain.LoadSynchronous();
		//Find the stage containing this quest
		for(int32 FoundStage = 0; FoundStage < CurrentChain->Stages.Num(); FoundStage++)
		{
			if(CurrentChain->Stages[FoundStage].Quests.Contains(Quest))
			{
				//Quest is part of the root stage required to start this chain.
				//It has no required quests.
				if(FoundStage == 0)
				{
					break;
				}
				
				//Reverse loop to the previous stages.
				//If any aren't completed, the required quests haven't been completed.
				for(int32 ScannedStages = FoundStage - 1; ScannedStages >= 0; ScannedStages--)
				{
					for(auto& CurrentQuest : CurrentChain->Stages[ScannedStages].Quests)
					{
						if(GetQuestState(CurrentQuest) != EBTQuestState::Completed)
						{
							return false;
						}
					}
				}
			}
		}
	}
	
	return true;
}

FBTQuestWrapper UQuestSystem::GetQuestForObjective(FGameplayTag Objective)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetQuestForObjective)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return FBTQuestWrapper();
	}

	for(auto& CurrentQuest : QuestSubSystem->Quests)
	{
		for(auto& CurrentStage : CurrentQuest.Value.ObjectiveStages)
		{
			for(auto& CurrentObjective : CurrentStage.Objectives)
			{
				if(CurrentObjective.ObjectiveID == Objective)
				{
					return CurrentQuest.Value;
				}
			}
		}
	}
	
	return FBTQuestWrapper();
}

FQuestObjective UQuestSystem::GetObjectiveByID(FGameplayTag ObjectiveID)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectiveByID)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return FQuestObjective();
	}

	FBTQuestWrapper FoundQuest = QuestSubSystem->GetQuestForObjective(ObjectiveID);
	if(!FoundQuest.QuestAsset)
	{
		return FQuestObjective();
	}

	for(auto& CurrentStage : FoundQuest.ObjectiveStages)
	{
		for(auto& CurrentObjective : CurrentStage.Objectives)
		{
			if(CurrentObjective.ObjectiveID == ObjectiveID)
			{
				return CurrentObjective;
			}
		}
	}

	return FQuestObjective();
}

EBTQuestState UQuestSystem::GetObjectiveState(FGameplayTag Objective)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetObjectiveState)
	
	FQuestObjective QuestObjective = GetObjectiveByID(Objective);
	if(QuestObjective.IsValid())
	{
		return QuestObjective.State;
	}

	return EBTQuestState::Inactive;
}

bool UQuestSystem::CompleteObjective(FGameplayTag ObjectiveID, UObject* Instigator)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CompleteObjective)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}
	
	FBTQuestWrapper FoundQuest = QuestSubSystem->GetQuestForObjective(ObjectiveID);
	if(!FoundQuest.QuestAsset)
	{
		return false;
	}

	FQuestObjective Objective = GetObjectiveByID(ObjectiveID);
	if(Objective.State == EBTQuestState::InProgress)
	{
		ProgressObjective(ObjectiveID, Objective.ProgressRequired - Objective.CurrentProgress, Instigator);
		return true;
	}
	
	return false;
}

bool UQuestSystem::ProgressObjective(const FGameplayTag ObjectiveID, float ProgressToAdd, UObject* Instigator)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProgressObjective)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}

	FBTQuestWrapper Quest = QuestSubSystem->GetQuestForObjective(ObjectiveID);

	FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(Quest.QuestAsset);
	if(!QuestWrapper)
	{
		return false;
	}

	bool ObjectiveCompleted = false;
	bool QuestCompleted = true;
	bool ObjectiveProgressed = false;

	/**Find the stage that the objective resides in. Then progress the objective.
	 * This will also check if all stages are now labelled complete, and if so,
	 * complete the quest. */
	for(int32 i = 0; i < QuestWrapper->ObjectiveStages.Num(); i++)
	{
		if(i == QuestWrapper->CurrentStage)
		{
			for(auto& CurrentObjective : QuestWrapper->ObjectiveStages[i].Objectives)
			{
				if(!CanObjectiveBeProgressed(CurrentObjective))
				{
					return false;
				}
			
				const float ProgressDelta = (FMath::Clamp(CurrentObjective.CurrentProgress + ProgressToAdd, 0, CurrentObjective.ProgressRequired) - CurrentObjective.CurrentProgress);

				CurrentObjective.CurrentProgress = FMath::Clamp(CurrentObjective.CurrentProgress + ProgressToAdd,0, CurrentObjective.ProgressRequired);
				ObjectiveProgressed = true;

				if(CurrentObjective.CurrentProgress == CurrentObjective.ProgressRequired)
				{
					CurrentObjective.State = EBTQuestState::Completed;
					ObjectiveCompleted = true;
					#if TAGFACTS_INSTALLED
					{
						/**If TagFacts is installed, we increment a fact by one.
						 * This fact matches the Objective ID, so we can track if
						 * this objective was completed through the fact system.*/
						UFactSubSystem::Get()->IncrementFact(CurrentObjective.ObjectiveID);
					}
					#endif
				}
			
				QuestSubSystem->ObjectiveProgressed.Broadcast(CurrentObjective, ProgressDelta, ObjectiveCompleted, Instigator);

				#if AsyncMessageSystem_Enabled
				if(TSharedPtr<FAsyncMessageSystemBase> Sys = UAsyncMessageWorldSubsystem::GetSharedMessageSystem(QuestSubSystem->GetWorld()))
				{
					Sys->QueueMessageForBroadcast(
						FAsyncMessageId(CurrentObjective.ObjectiveID), 
						FInstancedStruct::Make(CurrentObjective));
				}
				#endif

				#if ENABLE_VISUAL_LOG
				{
					UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
						10, FColor::White, TEXT("Progressed objective %s - %s / %s"),
						*ObjectiveID.ToString(),
						*FString::SanitizeFloat(CurrentObjective.CurrentProgress),
						*FString::SanitizeFloat(CurrentObjective.ProgressRequired));
				}
				#endif

				if(QuestWrapper->ObjectiveStages[i].IsComplete())
				{
					QuestWrapper->ObjectiveStages[i].IsActive = false;
					
					if(QuestWrapper->ObjectiveStages.IsValidIndex(i + 1))
					{
						//Label the next stage as the active one
						QuestWrapper->ObjectiveStages[i + 1].IsActive = true;
					}
					
					QuestSubSystem->QuestObjectiveStageCompleted.Broadcast(QuestWrapper->ObjectiveStages[i],
						QuestWrapper->ObjectiveStages.IsValidIndex(i + 1) ? QuestWrapper->ObjectiveStages[i + 1] : FQuestObjectiveStage());
				}

				//We don't break because we need to check if the quest is completed
			}
		}

		if(!QuestWrapper->ObjectiveStages[i].IsComplete())
		{
			//At least one stage is not complete. Don't complete the quest
			QuestCompleted = false;
		}
	}

	// for(auto& CurrentObjective : QuestWrapper->ObjectiveStages[QuestWrapper->CurrentStage].Objectives)
	// {
	// 	if(CurrentObjective.ObjectiveID == ObjectiveID)
	// 	{
	// 		if(!CanObjectiveBeProgressed(CurrentObjective))
	// 		{
	// 			return false;
	// 		}
	// 		
	// 		const float ProgressDelta = (FMath::Clamp(CurrentObjective.CurrentProgress + ProgressToAdd, 0, CurrentObjective.ProgressRequired) - CurrentObjective.CurrentProgress);
	//
	// 		CurrentObjective.CurrentProgress = FMath::Clamp(CurrentObjective.CurrentProgress + ProgressToAdd,0, CurrentObjective.ProgressRequired);
	// 		ObjectiveProgressed = true;
	//
	// 		if(CurrentObjective.CurrentProgress == CurrentObjective.ProgressRequired)
	// 		{
	// 			CurrentObjective.State = EBTQuestState::Completed;
	// 			ObjectiveCompleted = true;
	// 			#if TAGFACTS_INSTALLED
	// 			{
	// 				/**If TagFacts is installed, we increment a fact by one.
	// 				 * This fact matches the Objective ID, so we can track if
	// 				 * this objective was completed through the fact system.*/
	// 				UFactSubSystem::Get()->IncrementFact(CurrentObjective.ObjectiveID);
	// 			}
	// 			#endif
	// 		}
	// 		
	// 		QuestSubSystem->ObjectiveProgressed.Broadcast(CurrentObjective, ProgressDelta, ObjectiveCompleted, Instigator);
	// 		
	// 		#if HERMES_INSTALLED
	// 		{
	// 			UHermesSubsystem* Hermes =UHermesSubsystem::Get();
	// 			Hermes->BroadcastMessage(QuestSubSystem,CurrentObjective.ObjectiveID, FInstancedStruct::Make(CurrentObjective));
	// 		}
	// 		#endif
	//
	// 		#if ENABLE_VISUAL_LOG
	// 		{
	// 			UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
	// 				10, FColor::White, TEXT("Progressed objective %s - %s / %s"),
	// 				*ObjectiveID.ToString(),
	// 				*FString::SanitizeFloat(CurrentObjective.CurrentProgress),
	// 				*FString::SanitizeFloat(CurrentObjective.ProgressRequired));
	// 		}
	// 		#endif
	//
	// 		//We don't break because we need to check if the quest is completed
	// 	}
	//
	// 	if(CurrentObjective.State == EBTQuestState::InProgress && CurrentObjective.IsOptional != true)
	// 	{
	// 		QuestCompleted = false;
	// 	}
	// }

	if(QuestCompleted)
	{
		/**Every stage reported itself as "Complete", which means
		 * the quest should be completed. */
		CompleteQuest(QuestWrapper->QuestAsset, false);
	}

	return ObjectiveProgressed;
}

bool UQuestSystem::CanObjectiveBeProgressed(FQuestObjective Objective)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CanObjectiveBeProgressed)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}

	if(!Objective.IsValid())
	{
		return false;
	}

	if(Objective.State != EBTQuestState::InProgress)
	{
		return false;
	}


	if(Objective.CurrentProgress >= Objective.ProgressRequired)
	{
		return false;
	}

	return true;
}

bool UQuestSystem::FailObjective(FGameplayTag Objective, bool bFailQuest)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FailObjective)
	
	UQuestSystem* QuestSubSystem = UQuestSystem::Get();
	if(!QuestSubSystem)
	{
		return false;
	}

	const FBTQuestWrapper ObjectivesQuest = GetQuestForObjective(Objective);

	FBTQuestWrapper* QuestWrapper = QuestSubSystem->Quests.Find(ObjectivesQuest.QuestAsset);
	if(!QuestWrapper)
	{
		return false;
	}

	bool ObjectiveFailed = false;

	for(auto& CurrentStage : QuestWrapper->ObjectiveStages)
	{
		for(auto& CurrentObjective : CurrentStage.Objectives)
		{
			if(CurrentObjective.ObjectiveID == Objective)
			{
				CurrentObjective.State = EBTQuestState::Failed;

				#if ENABLE_VISUAL_LOG
				{
					UE_VLOG_LOCATION(QuestSubSystem, TEXT("Quest System %s"), Verbose, UGameplayStatics::GetPlayerPawn(QuestSubSystem, 0)->GetActorLocation(),
						10, FColor::White, TEXT("Failed Objective: %s"),
						*Objective.ToString());
				}
				#endif

				QuestSubSystem->ObjectiveFailed.Broadcast(CurrentObjective);
				
				#if AsyncMessageSystem_Enabled
				if(TSharedPtr<FAsyncMessageSystemBase> Sys = UAsyncMessageWorldSubsystem::GetSharedMessageSystem(QuestSubSystem->GetWorld()))
				{
					Sys->QueueMessageForBroadcast(
						FAsyncMessageId(CurrentObjective.ObjectiveID), 
						FInstancedStruct::Make(CurrentObjective));
				}
				#endif
				
				ObjectiveFailed = true;
				break;
			}
		}
	}

	if(bFailQuest)
	{
		FailQuest(QuestWrapper->QuestAsset,
			false /*Since we are failing a specific objective, don't go ahead and fail the others.*/);
	}
	else
	{
		/**This will check if the quest can be completed.
		 * If it can, then it will go ahead and complete it. */
		CompleteQuest(QuestWrapper->QuestAsset, false, false);
	}

	return ObjectiveFailed;
}

FBTQuestWrapper UQuestSystem::CreateQuestWrapper(TSoftObjectPtr<UQuestAsset> QuestAsset)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CreateQuestWrapper)
	
	FBTQuestWrapper QuestWrapper = FBTQuestWrapper();

	if(QuestAsset.IsNull())
	{
		return QuestWrapper;
	}
	
	QuestWrapper.QuestAsset = QuestAsset;
	QuestWrapper.State = EBTQuestState::InProgress;
	if(QuestAsset.LoadSynchronous()->ObjectiveStages.IsValidIndex(0))
	{
		QuestWrapper.ObjectiveStages = QuestAsset.LoadSynchronous()->ObjectiveStages;
		QuestWrapper.ObjectiveStages[0].IsActive = true;
		for(auto& CurrentObjective : QuestWrapper.ObjectiveStages[0].Objectives)
		{
			CurrentObjective.State = EBTQuestState::InProgress;
			CurrentObjective.RootQuest = QuestAsset;
		}
	}

	return QuestWrapper;
}
