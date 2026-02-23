// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Developer/I_AssetDetails.h"
#include "Engine/DataAsset.h"
#include "QuestAsset.generated.h"

class UQuestChain;
class UQuestRequirementBase;
class UQuestAsset;

namespace BTE
{
	static FName QuestID_Tag = FName("QuestID_Tag");
	static FName QuestName_Tag = FName("QuestName_Tag");
	static FName QuestChainName_Tag = FName("QuestChainName_Tag");
}

UENUM(BlueprintType)
enum class EBTQuestState : uint8
{
	//The player has not interacted
	//with this quest or task yet.
	Inactive,
	/**Mostly used for tasks that don't have
	 *their requirements met just yet. */
	Locked, 
	InProgress UMETA(DisplayName = "In Progress"),
	Completed,
	Failed
};

#pragma region QuestObjective
USTRUCT(BlueprintType)
struct FQuestObjective
{
	GENERATED_BODY()

	//The quest this objective belongs to.
	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UQuestAsset> RootQuest = nullptr;

	/**An ID for this objective so we can retrieve it later*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories="QuestSystem.Quests"))
	FGameplayTag ObjectiveID;
 
	/**Name of the objective presented to the player*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ObjectiveName;

	/**How much progress does this objective require?*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ProgressRequired = 1;

	UPROPERTY(BlueprintReadOnly)
	float CurrentProgress = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsOptional = false;

	/**What state is the task currently in?*/
	UPROPERTY(Category = "Task", BlueprintReadOnly)
	EBTQuestState State = EBTQuestState::Inactive;

	/**Arbitrary tags to associate with this objective.*/
	UPROPERTY(Category = "Task", EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer Tags;

	/**Which other objectives inside the owning quest must be
	 * completed before this task can be progressed?
	 *
	 * You must have other objectives available for this
	 * to work.
	 *
	 * This can be used to enable scenarios such as:
	 * - Collect 5 Apples
	 * - Collect 3 Carrots
	 * - Return the apples and carrots to Farmer John.
	 *
	 * This objective would be the last one, while the
	 * first 2 would be the dependency. */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly)
	// FGameplayTagContainer ObjectiveDependencies;

	bool IsValid() const
	{
		return ObjectiveID.IsValid() && !RootQuest.IsNull();
	}
};

USTRUCT(BlueprintType)
struct FQuestObjectiveStage
{
	GENERATED_BODY()

	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	TArray<FQuestObjective> Objectives;

	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	bool IsActive = false;

	bool IsComplete() const
	{
		for(auto& CurrentObject : Objectives)
		{
			if(CurrentObject.State == EBTQuestState::InProgress)
			{
				return false;
			}
		}

		return true;
	}
};

#pragma endregion

/**Wrapper struct for simple serialization and data management
 * revolving a quest, such as its current progress and state. */
USTRUCT(BlueprintType)
struct FBTQuestWrapper
{
	GENERATED_BODY()

	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UQuestAsset> QuestAsset = nullptr;

	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	TArray<FQuestObjectiveStage> ObjectiveStages;

	/**What state is the quest currently in?*/
	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	EBTQuestState State = EBTQuestState::Inactive;

	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	int32 CurrentStage = 0;

	bool operator==(const FBTQuestWrapper& Argument) const
	{
		return QuestAsset == Argument.QuestAsset;
	}

	bool operator!=(const FBTQuestWrapper& Argument) const
	{
		return QuestAsset != Argument.QuestAsset;
	}
};
/**
 * An asset that stores all data revolving a quest
 */
UCLASS()
class BT_QUESTS_API UQuestAsset : public UPrimaryDataAsset, public II_AssetDetails
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly, meta=(Categories="QuestSystem.Quests"))
	FGameplayTag QuestID;

	/**Name of the quest presented to the player*/
	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	FText QuestName;
	
	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly, meta = (MultiLine))
	FText QuestText;
	
	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly, meta=(Categories="QuestSystem.Quest.Type"))
	FGameplayTag QuestType;
	
	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	TArray<FQuestObjectiveStage> ObjectiveStages;

	/**Requirements to accept the quest.*/
	UPROPERTY(Category = "Quest", EditAnywhere, Instanced, BlueprintReadOnly)
	TArray<UQuestRequirementBase*> Requirements;

	UPROPERTY(Category = "Quest", BlueprintReadOnly)
	TArray<TSoftObjectPtr<UQuestChain>> QuestChains;

	/**If true, the currently tracked quest should automatically
	 * be changed to this. If false, the player has to manually track
	 * this quest. This is mostly just used for the tutorial
	 * or scenarios where you know the player has to do this quest
	 * after completing a previous one.
	 * For example; the player is on a quest where they are locked into
	 * a grand gameplay sequence, like a final mission.*/
	UPROPERTY(Category = "Quest", EditAnywhere, BlueprintReadOnly)
	bool AutoTrack = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

	virtual bool AppearsInContextMenu_Implementation() const override
	{
		return GetClass() == UQuestAsset::StaticClass();
	}

	virtual TArray<FText> GetAssetsCategories_Implementation() const override
	{
		return { FText::FromString("Quest System") };
	}
};
