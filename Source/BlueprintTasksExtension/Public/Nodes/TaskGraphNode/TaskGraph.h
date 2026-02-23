// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BtfTaskForge.h"
#include "GameplayTagContainer.h"
#include "Developer/I_AssetDetails.h"
#include "Engine/CancellableAsyncAction.h"

#include "TaskGraph.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGraphFinished, FGameplayTagContainer, FinishReasons);

/**
* TaskGraph's is a simple concept where a portion of the game is
* wrapped up into one or several blueprints.
* For example; the tutorial could be wrapped into a blueprint,
* where numerous events are tracked through tasks and gameplay events.
*
* Some task graphs can become large as they start referencing numerous
* tasks and potentially assets.
* This is why it's heavily encouraged to only soft reference
* task graphs and use the AsyncActivateTaskGraphNode.
* By default, you can only create task graphs through this
* async function to prevent hard reference mistakes. */
UCLASS(Abstract, Blueprintable)
class BLUEPRINTTASKSEXTENSION_API UTaskGraph : public UObject, public FTickableGameObject, public II_AssetDetails
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Task Graph", BlueprintImplementableEvent)
	void StartGraph();

	UPROPERTY(Category = "Task Graph", BlueprintAssignable)
	FGraphFinished GraphFinished;

	void FinishGraph()
	{
		FinishGraph(FGameplayTagContainer());
	}

	UFUNCTION(Category = "Task Graph", BlueprintCallable)
	void FinishGraph(FGameplayTagContainer FinishReasons)
	{
		GraphFinished.Broadcast(FinishReasons);

		// UBlueprintTaskTemplate::DeactivateAllTasksRelatedToObject(this);
		
		TArray<UObject*> SubObjects;
		GetObjectsWithOuter(this, SubObjects);

		/**Deactivate all tasks and cancel all async actions*/
		for(auto& CurrentObject : SubObjects)
		{
			if(UBtf_TaskForge* TaskTemplate = Cast<UBtf_TaskForge>(CurrentObject))
			{
				TaskTemplate->Deactivate();
			}
			else if(UCancellableAsyncAction* CancellableTask = Cast<UCancellableAsyncAction>(CurrentObject))
			{
				CancellableTask->Cancel();
			}
		}

		MarkAsGarbage();
	}

	/**Climbs the outer-chain until it finds an actor. This means that
	 * the owner does not equal this objects outer.
	 * For example, this might be a task graph inside another task graph,
	 * and that task graph is now our outer, that that task graph might
	 * have an actor set as its outer. This will then return that actor. */
	UFUNCTION(Category = "Task Graph", BlueprintCallable)
	AActor* GetOwningActor() const;

	/**Tick and timer support.
	 * Do mind that this asset should not use the tick event.
	 * It's only here to enable things like delays, async tasks, etc. */
	virtual void Tick(float DeltaTime) override {}
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	virtual class UWorld* GetWorld() const override;
	
	virtual FLinearColor GetAssetColor_Implementation() const override
	{
		return OmniToolbox::BlueprintAssetColor;
	}

	virtual bool AppearsInContextMenu_Implementation() const override
	{
		return GetClass() == UTaskGraph::StaticClass();
	}
};
