#include "Affix/AffixInventoryComponent.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UAffixInventoryComponent::UAffixInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAffixInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	BindSubsystem();
	RefreshOwned();
}

void UAffixInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSubsystem();
	Super::EndPlay(EndPlayReason);
}

bool UAffixInventoryComponent::AddAffix(UAffixDefinition* Definition)
{
	UAffixSubsystem* Affix = Resolve();
	if (!Affix || !Definition)
	{
		return false;
	}
	return Affix->GrantAffix(Definition->AffixId);
}

int32 UAffixInventoryComponent::GetStacks(FName AffixId) const
{
	for (const FAffixRuntimeEntry& Entry : Owned)
	{
		if (Entry.AffixId == AffixId)
		{
			return Entry.Stacks;
		}
	}
	if (const UAffixSubsystem* Affix = Resolve())
	{
		return Affix->GetAffixStacks(AffixId);
	}
	return 0;
}

void UAffixInventoryComponent::RefreshOwned()
{
	Owned.Reset();
	if (UAffixSubsystem* Affix = Resolve())
	{
		Owned = Affix->GetOwnedAffixes();
	}
}

void UAffixInventoryComponent::HandleAffixChanged(FAffixChangeEvent Event)
{
	RefreshOwned();
	OnAffixChanged.Broadcast(Event);
}

void UAffixInventoryComponent::HandleStatsRebuilt()
{
	RefreshOwned();
}

void UAffixInventoryComponent::BindSubsystem()
{
	if (Bound)
	{
		return;
	}
	UAffixSubsystem* Affix = Resolve();
	if (!Affix)
	{
		return;
	}
	Affix->OnAffixChanged.AddDynamic(this, &UAffixInventoryComponent::HandleAffixChanged);
	Affix->OnStatsRebuilt.AddDynamic(this, &UAffixInventoryComponent::HandleStatsRebuilt);
	Bound = Affix;
}

void UAffixInventoryComponent::UnbindSubsystem()
{
	if (!Bound)
	{
		return;
	}
	Bound->OnAffixChanged.RemoveDynamic(this, &UAffixInventoryComponent::HandleAffixChanged);
	Bound->OnStatsRebuilt.RemoveDynamic(this, &UAffixInventoryComponent::HandleStatsRebuilt);
	Bound = nullptr;
}

UAffixSubsystem* UAffixInventoryComponent::Resolve() const
{
	return UAffixSubsystemDefault::Get(this);
}
