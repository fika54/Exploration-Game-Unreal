#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CombatInterface.h"
#include "ProtagonistCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

class UCombatDefenceComponent;
class UCombatMeleeComponent;
class UCombatRangedComponent;
class UCombatAttackComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogProtagonistCharacter, Log, All);

UCLASS(abstract)
class ATTEMPT1_API AProtagonistCharacter : public ACharacter, public ICombatInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
	UInputAction* PrimaryAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
	UInputAction* SecondaryAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
	UInputAction* BlockAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
	UInputAction* DodgeAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatDefenceComponent* Defence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatMeleeComponent* Melee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatRangedComponent* Ranged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatAttackComponent* ActiveAttack;

public:
	AProtagonistCharacter();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void OnPrimary();
	void OnSecondary();
	void OnBlockStart();
	void OnBlockEnd();
	void OnDodge();

public:
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat") void EquipMelee();
	UFUNCTION(BlueprintCallable, Category = "Combat") void EquipRanged();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies") void AN_MeleeLightTrace();
	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies") void AN_MeleeHeavyTrace();

	// ICombatInterface
	virtual void TryAttack_Implementation(EAttackType Type) override;
	virtual void PerformAttackTrace_Implementation(const FAttackTraceSpec& Spec) override;
	virtual void OnDealtDamage_Implementation(AActor* Target, float Damage, const FVector& ImpactPoint) override {}
	virtual void StartBlock_Implementation() override;
	virtual void EndBlock_Implementation() override;
	virtual float GetParryWindowSeconds_Implementation() const override;
	virtual FDamageResult HandleIncomingAttack_Implementation(AActor* Attacker, float IncomingDamage,
		const FVector& ImpactPoint, const FVector& ImpulseDir) override;
	virtual void ApplyRawDamage_Implementation(float Damage, AActor* Causer, const FVector& ImpactPoint, const FVector& ImpulseDir) override;

public:
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
