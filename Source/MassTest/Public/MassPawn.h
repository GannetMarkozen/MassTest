
#pragma once

#include "MassEntityTypes.h"
#include "GameFramework/Pawn.h"
#include "MassPawn.generated.h"

class UInputAction;
class UMassEntityConfigAsset;
class UInputMappingContext;
struct FInputActionValue;

USTRUCT(BlueprintType)
struct MASSTEST_API FMassPawnStatics
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputMappingContext* InputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* MoveAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* LookAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* JumpAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UMassEntityConfigAsset* MassEntityConfig = nullptr;
};

UCLASS(Blueprintable, BlueprintType, SparseClassDataTypes="MassPawnStatics")
class MASSTEST_API AMassPawn : public APawn
{
	GENERATED_BODY()
public:
	explicit AMassPawn(const FObjectInitializer& ObjectInitializer);

protected:
	FMassEntityHandle EntityHandle;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJump();
};