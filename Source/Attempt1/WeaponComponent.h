// WeaponComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimTypes.h"                 // *** FIX: for FBranchingPointNotifyPayload
#include "WeaponComponent.generated.h"

// ------------------------------------------------------------
// Global types (must NOT be inside a class)
// ------------------------------------------------------------

// High-level type of weapon; extend as you like.
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Unarmed     UMETA(DisplayName = "Unarmed"),
    Sword       UMETA(DisplayName = "Sword"),
    Scythe      UMETA(DisplayName = "Scythe"),
    Gun         UMETA(DisplayName = "Gun"),
    Staff       UMETA(DisplayName = "Staff"),
    Other       UMETA(DisplayName = "Other")
};

// *** FIX: move to global scope
UENUM(BlueprintType)
enum class EWeaponLocomotionSet : uint8
{
    Default     UMETA(DisplayName = "Default"),
    Combat      UMETA(DisplayName = "Combat")
};

// *** FIX: delegate typedef must also be at global scope
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocomotionSetChanged, EWeaponLocomotionSet, NewSet);

// ------------------------------------------------------------

USTRUCT(BlueprintType)
struct FWeaponStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
    float BaseDamage = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
    float AttackPlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Stats")
    float AttackForce = 250.0f;
};

USTRUCT(BlueprintType)
struct FWeaponMeleeTraceSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee Trace", meta = (ClampMin = 0, Units = "cm"))
    float Distance = 75.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee Trace", meta = (ClampMin = 0, Units = "cm"))
    float Radius = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee Trace")
    bool bUseSocket = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Melee Trace")
    FName SocketOrBone = FName("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Debug")
    bool bDebug = false;
};

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ATTEMPT1_API UWeaponComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponComponent();

    // ------------ Core identity ------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    EWeaponType WeaponType = EWeaponType::Unarmed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Attach")
    FName HandSocketName = FName("hand_r");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Attach")
    FName HolsterSocketName = FName("spine_socket");

    // Optional weapon meshes (not required for unarmed)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Mesh")
    TObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Mesh")
    TObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Mesh")
    TSubclassOf<UAnimInstance> WeaponAnimClassHolstered;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Mesh")
    TSubclassOf<UAnimInstance> WeaponAnimClassUnholstered;

    // ------------ Stats & tuning ------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FWeaponStats Stats;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Melee")
    FWeaponMeleeTraceSpec LightTraceSpec;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Melee")
    FWeaponMeleeTraceSpec HeavyTraceSpec;

    // Ranged (optional)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
    TSubclassOf<AActor> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
    FName MuzzleSocketName = FName("muzzle");

    // ------------ Animations (match CombatCharacter) ------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Combo")
    UAnimMontage* ComboAttackMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Combo")
    TArray<FName> ComboSectionNames;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Combo", meta = (ClampMin = 0, ClampMax = 5))
    float ComboInputCacheTimeTolerance = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Charged")
    UAnimMontage* ChargedAttackMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Charged")
    FName ChargeLoopSection = FName("Charge_Loop");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Charged")
    FName ChargeAttackSection = FName("Charge_Attack");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|General", meta = (ClampMin = 0, ClampMax = 5))
    float AttackInputCacheTimeTolerance = 1.0f;

    // Equip/unequip (optional)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Equip")
    TObjectPtr<UAnimMontage> UnholsterMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Equip")
    TObjectPtr<UAnimMontage> HolsterMontage;

    // ------------ (Optional) Owner anim layers / locomotion ------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Owner")
    TSubclassOf<UAnimInstance> OwnerWeaponLayerClass;  // linked via LinkAnimClassLayers

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Owner")
    bool bOverrideOwnerSpeedsOnEquip = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Owner", meta = (EditCondition = "bOverrideOwnerSpeedsOnEquip"))
    float CombatWalkSpeed = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Owner", meta = (EditCondition = "bOverrideOwnerSpeedsOnEquip"))
    float CombatRunSpeed = 450.f;

    // *** FIX: use global types declared above
    UPROPERTY(BlueprintAssignable, Category = "Anim|Owner")
    FOnLocomotionSetChanged OnLocomotionSetChanged;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anim|Owner")
    EWeaponLocomotionSet LocomotionSet = EWeaponLocomotionSet::Combat;

    // ------------ Runtime state ------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|State")
    bool bIsEquipped = false;

    // Public API
    UFUNCTION(BlueprintCallable, Category = "Weapon|Equip")
    void EquipWeapon(bool bPlayMontage = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon|Equip")
    void HolsterWeapon(bool bPlayMontage = true);

    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetWeaponType(EWeaponType NewType) { WeaponType = NewType; }

    UFUNCTION(BlueprintCallable, Category = "Weapon|Attack")
    virtual void PrimaryAttack();              // combo

    UFUNCTION(BlueprintCallable, Category = "Weapon|Attack")
    virtual void SecondaryAttack_Start();      // charge hold

    UFUNCTION(BlueprintCallable, Category = "Weapon|Attack")
    virtual void SecondaryAttack_Release();    // charge release

    // Anim Notifies call these
    UFUNCTION(BlueprintCallable, Category = "Weapon|AnimNotify")
    void AN_CheckComboWindow();

    UFUNCTION(BlueprintCallable, Category = "Weapon|AnimNotify")
    void AN_CheckChargedAttack();

    UFUNCTION(BlueprintCallable, Category = "Weapon|AnimNotify")
    void AN_DoLightTrace(FName DamageSourceBone);

    UFUNCTION(BlueprintCallable, Category = "Weapon|AnimNotify")
    void AN_DoHeavyTrace(FName DamageSourceBone);

protected:
    virtual void BeginPlay() override;

    void RefreshOwnerPointers();
    void EnsureWeaponComponents();
    void AttachToOwnerSocket(const FName& SocketName);
    void ApplyWeaponAnimClass(bool bHolsteredState);
    void PlayOwnerMontage(UAnimMontage* Montage, float PlayRate = 1.f) const;

    void StartCombo();
    void StartCharged();
    void AttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    void PerformMeleeTrace(const FWeaponMeleeTraceSpec& Spec, FName DamageSourceBone);

    // *** FIX: notify binding function signature
    UFUNCTION()
    void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& Payload);

    // Owner anim helpers (optional)
    void ApplyOwnerAnimLayers(bool bLink);
    void ApplyOwnerLocomotionOnEquip();
    void RestoreOwnerLocomotionOnHolster();

private:
    // Owner pointers
    UPROPERTY(Transient) class ACharacter* OwnerChar = nullptr;
    UPROPERTY(Transient) class USkeletalMeshComponent* OwnerMesh = nullptr;
    UPROPERTY(Transient) class UAnimInstance* OwnerAnim = nullptr;

    // Optional weapon meshes
    UPROPERTY(Transient) class USkeletalMeshComponent* WeaponSK = nullptr;
    UPROPERTY(Transient) class UStaticMeshComponent* WeaponSM = nullptr;

    // State
    bool  bIsAttacking = false;
    bool  bIsChargingAttack = false;
    bool  bHasLoopedChargedAttack = false;
    int32 ComboIndex = 0;
    double CachedAttackInputTime = -1000.0;

    FOnMontageEnded OnAttackMontageEnded;

    // cached owner speeds (optional)
    float CachedWalkSpeed = 0.f;
    float CachedRunSpeed = 0.f;
};
