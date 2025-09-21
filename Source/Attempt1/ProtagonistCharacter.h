#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WeaponComponent.h"
#include "Logging/LogMacros.h"
#include "CombatInterface.h"

#include "EnhancedInputSubsystems.h"   // UEnhancedInputLocalPlayerSubsystem
#include "InputMappingContext.h"       // UInputMappingContext
#include "Engine/LocalPlayer.h"        // ULocalPlayer

#include "ProtagonistCharacter.generated.h"



class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

class UCombatDefenceComponent;
class UCombatMeleeComponent;
class UCombatRangedComponent;
class UCombatAttackComponent;

class UMobilityComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Combat")
	UInputAction* ToggleStrafeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Mode")
	UInputAction* ToggleMobilityAction;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatDefenceComponent* Defence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatMeleeComponent* Melee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatRangedComponent* Ranged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatAttackComponent* ActiveAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UWeaponComponent* ActiveWeapon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UWeaponComponent> DefaultWeaponClass;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mobility", meta = (AllowPrivateAccess = "true"))
	//UMobilityComponent* Mobility = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mode")
	bool bMobilityMode = false;

	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputMapping")
	TObjectPtr<UInputMappingContext> DefaultIMC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputMapping")
	int32 IMCPriority = 0;

	

public:
	AProtagonistCharacter();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void OnPrimary();
	void OnSecondaryStart();
	void OnSecondaryRelease();
	void OnToggleHolster();
	void OnBlockStart();
	void OnBlockEnd();
	void OnDodgeOrSprint();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void BeginPlay() override;

	//for experimentation
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoMove(float Right, float Forward);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void EnableStrafing();
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoLook(float Yaw, float Pitch);
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpStart();
	UFUNCTION(BlueprintCallable, Category = "Input") virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category = "Combat") void EquipMelee();
	UFUNCTION(BlueprintCallable, Category = "Combat") void EquipRanged();

	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies") void AN_MeleeLightTrace();
	UFUNCTION(BlueprintCallable, Category = "Combat|Notifies") void AN_MeleeHeavyTrace();

	UFUNCTION(BlueprintCallable, Category = "Weapon") void EquipDefaultWeapon(bool bPlayMontage = true);

	UFUNCTION(BlueprintCallable, Category = "Weapon") void EquipWeaponClass(TSubclassOf<UWeaponComponent> WeaponClass, bool bHolsterPrevious = true);

	UFUNCTION() void OnWeaponLocomotionChanged(EWeaponLocomotionSet NewSet);

	// --- Camera / Facing toggle ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bCameraStrafeMode = true; // true = face camera yaw & strafe

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ToggleCameraStrafeMode();

	// Set mode explicitly; bInstant forces an immediate snap to camera yaw
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraStrafeMode(bool bEnable, bool bInstant = false);

	// Optional: expose the current mode to AnimBP
	UPROPERTY(BlueprintReadOnly, Category = "Camera")
	bool bIsStrafingLocomotion = true;

	UFUNCTION()
	void AddDefaultIMC_Local();

	UFUNCTION()
	void RemoveDefaultIMC_Local();



	/*UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeaponClass(TSubclassOf<UWeaponComponent> WeaponClass, bool bPlayMontage = true);*/


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


	//for testing
private:
	// Base walking speed
	float WalkSpeed;

	// Sprinting speed
	float SprintSpeed;

	// Target speed for interpolation
	float TargetSpeed;

	// Interpolation speed (how fast to reach target speed)
	float SpeedInterpolationRate;

};
