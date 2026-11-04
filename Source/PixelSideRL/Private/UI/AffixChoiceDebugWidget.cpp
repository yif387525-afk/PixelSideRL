#include "UI/AffixChoiceDebugWidget.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UAffixChoiceDebugWidget::UAffixChoiceDebugWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UAffixChoiceDebugWidget::EnsureLayout()
{
	if (RootBox || !WidgetTree)
	{
		return;
	}
	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox_AffixChoice"));
	WidgetTree->RootWidget = RootBox;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Title"));
	Title->SetText(FText::FromString(TEXT("三选一  —  1 / 2 / 3")));
	RootBox->AddChildToVerticalBox(Title);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Btn_Card_%d"), Index));
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Txt_Card_%d"), Index));
		Line->SetAutoWrapText(true);
		Button->AddChild(Line);
		if (Index == 0)
		{
			Button->OnClicked.AddDynamic(this, &UAffixChoiceDebugWidget::HandleCard0);
		}
		else if (Index == 1)
		{
			Button->OnClicked.AddDynamic(this, &UAffixChoiceDebugWidget::HandleCard1);
		}
		else
		{
			Button->OnClicked.AddDynamic(this, &UAffixChoiceDebugWidget::HandleCard2);
		}
		if (UVerticalBoxSlot* Slot = RootBox->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.f, 6.f));
		}
		Lines.Add(Line);
	}
}

void UAffixChoiceDebugWidget::ShowChoices(const TArray<FAffixChoiceEntry>& Choices)
{
	EnsureLayout();
	UAffixSubsystem* Affix = UAffixSubsystemDefault::Get(this);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		UTextBlock* Line = Lines[Index];
		if (!Line)
		{
			continue;
		}
		if (!Choices.IsValidIndex(Index) || !Affix)
		{
			Line->SetText(FText::GetEmpty());
			continue;
		}
		const FAffixCardView Card = Affix->GetAffixCardView(Choices[Index].Def, 1);
		const FString Rarity = StaticEnum<EAffixRarity>()
			? StaticEnum<EAffixRarity>()->GetNameStringByValue(static_cast<int64>(Card.Rarity))
			: TEXT("Common");
		Line->SetText(FText::FromString(FString::Printf(
			TEXT("[%d] %s  (%s)\n%s"),
			Index + 1,
			*Card.Name.ToString(),
			*Rarity,
			*Card.Desc.ToString())));
	}
}

FReply UAffixChoiceDebugWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::One || Key == EKeys::NumPadOne)
	{
		ConfirmIndex(0);
		return FReply::Handled();
	}
	if (Key == EKeys::Two || Key == EKeys::NumPadTwo)
	{
		ConfirmIndex(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Three || Key == EKeys::NumPadThree)
	{
		ConfirmIndex(2);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAffixChoiceDebugWidget::ConfirmIndex(int32 Index)
{
	if (UAffixSubsystem* Affix = UAffixSubsystemDefault::Get(this))
	{
		Affix->ConfirmAffixChoice(Index);
	}
}

void UAffixChoiceDebugWidget::HandleCard0()
{
	ConfirmIndex(0);
}

void UAffixChoiceDebugWidget::HandleCard1()
{
	ConfirmIndex(1);
}

void UAffixChoiceDebugWidget::HandleCard2()
{
	ConfirmIndex(2);
}
