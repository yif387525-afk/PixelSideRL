#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/PixelEnemyGrunt.h"
#include "PixelRoomDirector.generated.h"

class UAffixSubsystem;

struct FPixelRoomDirectorPrivate;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPixelRoomWavesCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPixelRoomExitUnlocked);

UENUM(BlueprintType)
enum class EPixelRoomPhase : uint8
{
	Idle,
	Arming,
	Fighting,
	AwaitingAffix,
	Cleared
};

/**
 * P0.7 room clock for Room_Combat_01. Reads the corridor CSV, spawns APixelEnemyGrunt
 * for Enemy.Grunt.Early, then calls RequestAffixChoice(NormalRoom). Exit.Right opens
 * after the player confirms one card. Exit.Left does not rewind a clear.
 * Room_Combat_01.umap is not in this pass — bBuildGreyboxFromCsv lays the CSV geometry
 * in an empty map. See Docs/Combat_P0_LabSetup.md.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "RoomDirector"))
class PIXELSIDERL_API APixelRoomDirector : public AActor
{
	GENERATED_BODY()

public:
	APixelRoomDirector();
	virtual ~APixelRoomDirector() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "PixelSide|Room")
	void StartRoom();

	UFUNCTION(BlueprintCallable, Category = "PixelSide|Room")
	void NotifyExitTouched(FName InExitId);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	FName RoomId;

	/** Clamped to 0.5–1.0s. Default sits in the middle of the RoomPipeline window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float FirstWaveDelay = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bAutoStart = true;

	/** Turn off once Room_Combat_01.umap exists and the geometry is in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bBuildGreyboxFromCsv = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bPlacePlayerAtAnchor = true;

	/** Enemy.Grunt.Early. Empty falls back to APixelEnemyGrunt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	TSubclassOf<APixelEnemyGrunt> GruntEarlyClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	EPixelRoomPhase Phase = EPixelRoomPhase::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	int32 AliveCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	int32 CurrentWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bRoomCleared = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bRequireClearExitOpen = false;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|Room")
	FOnPixelRoomWavesCleared OnRoomWavesCleared;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|Room")
	FOnPixelRoomExitUnlocked OnRequireClearExitUnlocked;

private:
	UFUNCTION()
	void HandleAffixChoiceClosed(bool bConfirmed);

	void BindAffix();
	void UnbindAffix();
	void PollDeaths();
	void SpawnQueued();
	void OfferAffix();
	void Publish();
	void BuildGreybox();
	void PlacePlayer();
	UAffixSubsystem* ResolveAffix() const;

	FPixelRoomDirectorPrivate* Priv = nullptr;
	UAffixSubsystem* BoundAffix = nullptr;
	bool bAffixBound = false;
	bool bRoomStarted = false;
	bool bPlayerPlaced = false;
	FName LastTouchId;
	float LastTouchTime = -1000.f;
};
