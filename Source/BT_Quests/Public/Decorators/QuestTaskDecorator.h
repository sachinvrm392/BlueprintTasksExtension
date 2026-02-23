// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NodeDecorators/BtfNodeDecorator.h"
#include "QuestTaskDecorator.generated.h"

class UQuestAsset;
/**
 * 
 */
UCLASS()
class BT_QUESTS_API UQuestTaskDecorator : public UBtf_NodeDecorator
{
	GENERATED_BODY()
	
	TSharedPtr<SVerticalBox> CenterContentArea;
	TSharedPtr<STextBlock> CenterText;
	
	TWeakObjectPtr<UQuestAsset> QuestInformation = nullptr;

#if WITH_EDITOR
	virtual TSharedRef<SWidget> CreateCenterContent(UClass* TaskClass, UBtf_TaskForge* BlueprintTaskNode, UEdGraphNode* GraphNode) override;
#endif

	virtual TArray<UObject*> Get_ObjectsForExtraDetailsPanels() const override;
};
