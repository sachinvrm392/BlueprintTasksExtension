// Copyright (C) Varian Daemon 2025. All Rights Reserved.


#include "Tasks/QuestTaskNode.h"

#include "QuestSystem.h"
#include "Decorators/QuestTaskDecorator.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

UQuestTaskNode::UQuestTaskNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	Category = "Quests";
	Tooltip = "Task that accepts and tracks the progress of a quest";
	MenuDisplayName = "Quest Task";
	
	Decorator = UQuestTaskDecorator::StaticClass();

	if(HasAnyFlags(RF_ClassDefaultObject))
	{
		RefreshCollected();
		//By default, these will always appear
		SpawnParam.Add(FBtf_NameSelect("QuestAsset"));
		OutDelegate.Add(FBtf_NameSelect("QuestAccepted"));
		OutDelegate.Add(FBtf_NameSelect("QuestCompleted"));
		ExecFunction.Empty();
	}
#endif
}

void UQuestTaskNode::Activate_Internal()
{
	Super::Activate_Internal();

	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	Streamable.RequestAsyncLoad(QuestAsset.ToSoftObjectPath(), [this]
	{
		UQuestSystem* QuestSystem = UQuestSystem::Get();
		if(!QuestSystem)
		{
			return;
		}
		
		/**Setup the delegates so we can forward those events to the output pins*/
		QuestSystem->QuestCompleted.AddDynamic(this, &UQuestTaskNode::OnQuestCompleted);
		QuestSystem->QuestFailed.AddDynamic(this, &UQuestTaskNode::OnQuestFailed);
		QuestSystem->QuestAccepted.AddDynamic(this, &UQuestTaskNode::OnQuestAccepted);
		QuestSystem->QuestAbandoned.AddDynamic(this, &UQuestTaskNode::OnQuestAbandoned);
		QuestSystem->ObjectiveProgressed.AddDynamic(this, &UQuestTaskNode::OnQuestObjectiveCompleted);
		QuestSystem->QuestObjectiveStageCompleted.AddDynamic(this, &UQuestTaskNode::OnQuestObjectiveStageCompleted);

		EBTQuestState QuestState = QuestSystem->GetQuestState(QuestAsset);
		if(TriggerStatePinsIfQuestIsNotInactive)
		{
			switch(QuestState)
			{
				case EBTQuestState::Completed:
					QuestCompleted.Broadcast();
					break;
				case EBTQuestState::Failed:
					QuestFailed.Broadcast();
					break;
				default:
					break;
			}
		}
		
		if(AcceptQuestOnActivate && QuestSystem->CanAcceptQuest(QuestAsset))
		{
			if(!QuestSystem->AcceptQuest(QuestAsset, false))
			{
				QuestFailedRequirements.Broadcast();
			}
		}
	});
	
}

bool UQuestTaskNode::Get_NodeTitleColor_Implementation(FLinearColor& Color)
{
	Color = FLinearColor(FColor(243, 100, 0));
	return true;
}
