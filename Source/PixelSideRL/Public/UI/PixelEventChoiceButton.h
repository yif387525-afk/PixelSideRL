#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PixelEventChoiceButton.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPixelEventChoiceButtonClicked, FName, ChoiceId);

/**
 * One row inside WBP_EventChoice. Shows a hotkey digit and Choices[].Label.
 * Does not apply event effects.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_EventChoiceButton"))
class PIXELSIDERL_API UPixelEventChoiceButton : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelEventChoiceButton(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetOption(FName InChoiceId, const FText& InLabel, int32 InHotkey);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetFocused(bool bInFocused);

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	FName GetChoiceId() const { return ChoiceId; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnPixelEventChoiceButtonClicked OnChoiceClicked;

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Select;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Hotkey;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Label;

private:
	UFUNCTION()
	void HandleSelectClicked();

	void EnsureLayout();
	void BuildFallbackLayout();
	void BindSelectButton();

	FName ChoiceId = NAME_None;
	int32 HotkeyNumber = 1;
	bool bFocused = false;
	bool bWarnedMissingWidgets = false;

	/** Fallback-only 1px focus ring. Not a required WBP name. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> FocusRing;
};
