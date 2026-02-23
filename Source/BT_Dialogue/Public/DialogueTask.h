// Copyright (C) Varian Daemon 2025. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BtfTaskForge.h"
#include "GameplayTagContainer.h"
#include "Developer/I_AssetDetails.h"

#include "DialogueTask.generated.h"

class UDialogueCondition;
class UDialogueCharacter;

UENUM(BlueprintType)
enum class EDialogueConditionHandling : uint8
{
	AnyCondition,
	AllConditions
};

UENUM(BlueprintType)
enum class EDialogueType : uint8
{
	FullScreen,
	CornerPopup
};

USTRUCT(BlueprintType)
struct FDialogueConditionData
{
	GENERATED_BODY()

	/**If the owning dialogue option does NOT want to be hidden, but the requirements
	 * are still not met, the option will be greyed out and not selectable, but still
	 * visible to the player.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Option", meta = (EditCondition = "Conditions.IsEmpty() == true", EditConditionHides))
	bool HideIfConditionsAreNotMet = true;

	/**When it's time to present this option to the player, what conditions must
	 * be met?*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Dialogue Option")
	TArray<UDialogueCondition*> Conditions;
};

USTRUCT(BlueprintType)
struct FDialogueMemoryData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool RememberOptionSelection = false;

	/**If @RememberOptionSelection is true, AND TagFacts is installed,
	 * we will increment a fact with this tag by 1 to remember how
	 * often this option has been selected.
	 * If you don't have TagFacts installed, you must implement your
	 * own solution for storing this dialogue option in your
	 * own database. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!RememberOptionSelection"))
	FGameplayTag OptionFact;
};

/**V: For now, this is just a text field. It's being handled
 * this way so in the case of us wanting to expand this,
 * for example adding audio, we won't have to do any
 * PostLoad shenanigans. */
USTRUCT(BlueprintType)
struct FCharacterDialogueText
{
	GENERATED_BODY()

	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"))
	FText DialogueText = FText();
};

USTRUCT(BlueprintType)
struct FDialogueTaskOption : public FCustomOutputPinData
{
	GENERATED_BODY()

	/**The text displayed to the player to choose this option.*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Option")
	FText ButtonText = FText();

	/**The actual dialogue that occurs when the player presses
	 * the dialogue button.
	 * Each index is presented one-by-one. */
	UPROPERTY(Category = "Dialogue Option", EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "DialogueText"))
	TArray<FCharacterDialogueText> DialogueTexts;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Option")
	FDialogueConditionData ConditionSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Option")
	FDialogueMemoryData MemorySettings;
};

USTRUCT(BlueprintType)
struct FDialogueScript
{
	GENERATED_BODY()

	/**The character that is speaking.
	 * This is not set in stone during the dialogue. */
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UDialogueCharacter> Character = nullptr;

	/**The dialogue the character will speak.
	 * Each index is presented one-by-one. */
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite, meta = (TitleProperty = "DialogueText"))
	TArray<FCharacterDialogueText> CharacterDialogue;

	/**The options the player will get when the character is done speaking.
	 * This can be left empty, for example if the character has the final
	 * word and the player can only listen. */
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!NotInteractive", TitleProperty = "ButtonText"))
	TArray<FDialogueTaskOption> DialogueOptions;

	/**If set to true, this dialogue task won't display any dialogue options.
 	 * Allowing NPC's to yap without the player having to respond. */
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite)
	bool NotInteractive = false;

	/**After a dialogue option has been chosen, how long should we wait
	 * until we trigger the dialogue pin */
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadOnly)
	float FinishDelay = 0.4;
	
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite)
	EDialogueType DialogueType = EDialogueType::FullScreen;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDialogueFinished);

/**
 * A task that is responsible for handling interactive dialogue
 * scenes for the player.
 * The base class does not have any runtime logic. For this to work,
 * you must create a child and implement your own widgets and logic.
 * Your widget must have a delegate that will execute SelectDialogueOption.
 */
UCLASS(Abstract, AutoExpandCategories = ("Dialogue"))
class BT_DIALOGUE_API UDialogueTask : public UBtf_TaskForge, public II_AssetDetails
{
	GENERATED_BODY()

public:

	UDialogueTask(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite)
	FDialogueScript Script;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FDialogueFinished DialogueFinished;

	/**Set this to true if the dialogue screen
	 * is supposed to be removed when this
	 * dialogue task is finished.*/
	UPROPERTY(Category = "Dialogue", EditAnywhere, BlueprintReadWrite)
	bool RemoveDialogueScreenOnDeactivate = false;

	virtual bool Get_NodeTitleColor_Implementation(FLinearColor& Color) override;

	UFUNCTION(BlueprintNativeEvent)
	FString GetCenterText();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UTexture2D* GetSpeakerPortrait();

	virtual TArray<FCustomOutputPin> Get_CustomOutputPins_Implementation() const override;

	UFUNCTION(BlueprintCallable)
	void SelectDialogueOption(FDialogueTaskOption Option);

	virtual TArray<FString> ValidateNodeDuringCompilation_Implementation() override;
	
	virtual FLinearColor GetAssetColor_Implementation() const override
	{
		return OmniToolbox::BlueprintAssetColor;
	}

	virtual bool AppearsInContextMenu_Implementation() const override
	{
		return GetClass() == UDialogueTask::StaticClass();
	}

	virtual TArray<FText> GetAssetsCategories_Implementation() const override
	{
		return { FText::FromString("Dialogue System") };
	}
};
