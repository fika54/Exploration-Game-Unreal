// WeaponComponent.cpp

#include "WeaponComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"                // *** CHANGED: for ApplyPointDamage fallback
#include "CombatInterface.h"
#include "CombatDefenceComponent.h"                // *** CHANGED: prefer calling Defence component when present

UWeaponComponent::UWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    Stats.BaseDamage = 1.0f;
    Stats.AttackPlayRate = 1.0f;
    Stats.AttackForce = 250.0f;

    LightTraceSpec.Distance = 75.f;
    LightTraceSpec.Radius = 60.f;
    LightTraceSpec.bUseSocket = true;
    LightTraceSpec.SocketOrBone = FName("hand_r");

    HeavyTraceSpec = LightTraceSpec;
    HeavyTraceSpec.Distance = 90.f;
    HeavyTraceSpec.Radius = 75.f;

    OnAttackMontageEnded.BindUObject(this, &UWeaponComponent::AttackMontageEnded);
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshOwnerPointers();
    EnsureWeaponComponents();

    const bool bStartHolstered = true;
    ApplyWeaponAnimClass(bStartHolstered);
    AttachToOwnerSocket(bStartHolstered ? HolsterSocketName : HandSocketName);
    bIsEquipped = !bStartHolstered;
}

void UWeaponComponent::RefreshOwnerPointers()
{
    OwnerChar = Cast<ACharacter>(GetOwner());
    OwnerMesh = OwnerChar ? OwnerChar->GetMesh() : nullptr;
    OwnerAnim = OwnerMesh ? OwnerMesh->GetAnimInstance() : nullptr;
}

void UWeaponComponent::EnsureWeaponComponents()
{
    if (!OwnerChar) return;

    if (SkeletalMesh && !WeaponSK)
    {
        WeaponSK = NewObject<USkeletalMeshComponent>(OwnerChar);
        WeaponSK->RegisterComponent();
        WeaponSK->AttachToComponent(OwnerChar->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        WeaponSK->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponSK->SetGenerateOverlapEvents(false);
    }

    if (StaticMesh && !WeaponSM)
    {
        WeaponSM = NewObject<UStaticMeshComponent>(OwnerChar);
        WeaponSM->RegisterComponent();
        WeaponSM->AttachToComponent(OwnerChar->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        WeaponSM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        WeaponSM->SetGenerateOverlapEvents(false);
    }
}

void UWeaponComponent::AttachToOwnerSocket(const FName& SocketName)
{
    if (!OwnerMesh) return;

    if (WeaponSK)
    {
        WeaponSK->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    }
    if (WeaponSM)
    {
        WeaponSM->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    }
}

void UWeaponComponent::ApplyWeaponAnimClass(bool bHolsteredState)
{
    if (!WeaponSK) return;

    const TSubclassOf<UAnimInstance> AnimClass = bHolsteredState ? WeaponAnimClassHolstered : WeaponAnimClassUnholstered;
    if (AnimClass)
    {
        WeaponSK->SetAnimInstanceClass(AnimClass);
    }
}

void UWeaponComponent::PlayOwnerMontage(UAnimMontage* Montage, float PlayRate) const
{
    if (!Montage || !OwnerAnim) return;
    OwnerAnim->Montage_Play(Montage, PlayRate);
}

// -------------------- Owner Anim Layers and Locomotion --------------------

void UWeaponComponent::ApplyOwnerAnimLayers(bool bLink)
{
    if (!OwnerAnim || !OwnerWeaponLayerClass) return;

    if (bLink)
    {
        OwnerAnim->LinkAnimClassLayers(OwnerWeaponLayerClass);
    }
    else
    {
        OwnerAnim->UnlinkAnimClassLayers(OwnerWeaponLayerClass);
    }
}

void UWeaponComponent::ApplyOwnerLocomotionOnEquip()
{
    OnLocomotionSetChanged.Broadcast(LocomotionSet);

    if (bOverrideOwnerSpeedsOnEquip && OwnerChar)
    {
        if (UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement())
        {
            CachedWalkSpeed = Move->MaxWalkSpeedCrouched;
            CachedRunSpeed = Move->MaxWalkSpeed;

            Move->MaxWalkSpeed = CombatRunSpeed;
            Move->MaxWalkSpeedCrouched = CombatWalkSpeed;
        }
    }
}

void UWeaponComponent::RestoreOwnerLocomotionOnHolster()
{
    OnLocomotionSetChanged.Broadcast(EWeaponLocomotionSet::Default);

    if (bOverrideOwnerSpeedsOnEquip && OwnerChar)
    {
        if (UCharacterMovementComponent* Move = OwnerChar->GetCharacterMovement())
        {
            if (CachedRunSpeed > 0.f) Move->MaxWalkSpeed = CachedRunSpeed;
            if (CachedWalkSpeed > 0.f) Move->MaxWalkSpeedCrouched = CachedWalkSpeed;
        }
    }
}

// -------------------- Equip / Holster --------------------

void UWeaponComponent::EquipWeapon(bool bPlayMontage)
{
    RefreshOwnerPointers();
    EnsureWeaponComponents();

    // *** CHANGED [Notify Binding]: ensure our component receives montage notifies
    if (OwnerAnim && !OwnerAnim->OnPlayMontageNotifyBegin.IsAlreadyBound(this, &UWeaponComponent::OnMontageNotifyBegin))
    {
        OwnerAnim->OnPlayMontageNotifyBegin.AddDynamic(this, &UWeaponComponent::OnMontageNotifyBegin);
    }

    if (bPlayMontage && UnholsterMontage)
    {
        PlayOwnerMontage(UnholsterMontage, 1.0f * Stats.AttackPlayRate);
    }

    ApplyWeaponAnimClass(false);
    AttachToOwnerSocket(HandSocketName);

    ApplyOwnerAnimLayers(true);
    ApplyOwnerLocomotionOnEquip();

    bIsEquipped = true;
}

void UWeaponComponent::HolsterWeapon(bool bPlayMontage)
{
    RefreshOwnerPointers();
    EnsureWeaponComponents();

    if (bPlayMontage && HolsterMontage)
    {
        PlayOwnerMontage(HolsterMontage, 1.0f);
    }

    ApplyWeaponAnimClass(true);
    AttachToOwnerSocket(HolsterSocketName);

    ApplyOwnerAnimLayers(false);
    RestoreOwnerLocomotionOnHolster();

    bIsEquipped = false;
}

// -------------------- Attacks (montage+section flow) --------------------

void UWeaponComponent::PrimaryAttack()
{
    if (!OwnerAnim || !ComboAttackMontage || ComboSectionNames.Num() == 0) return;

    const double Now = GetWorld()->GetTimeSeconds();

    if (bIsAttacking)
    {
        CachedAttackInputTime = Now; // buffer
        return;
    }

    StartCombo();
}

void UWeaponComponent::SecondaryAttack_Start()
{
    if (!OwnerAnim || !ChargedAttackMontage) return;

    bIsChargingAttack = true;

    if (bIsAttacking)
    {
        CachedAttackInputTime = GetWorld()->GetTimeSeconds();
        return;
    }

    StartCharged();
}

void UWeaponComponent::SecondaryAttack_Release()
{
    bIsChargingAttack = false;

    if (bHasLoopedChargedAttack)
    {
        AN_CheckChargedAttack();
    }
}

void UWeaponComponent::StartCombo()
{
    if (!OwnerAnim || !ComboAttackMontage || ComboSectionNames.Num() == 0) return;

    bIsAttacking = true;
    ComboIndex = 0;

    const float Len = OwnerAnim->Montage_Play(ComboAttackMontage, Stats.AttackPlayRate, EMontagePlayReturnType::MontageLength, 0.f, true);
    if (Len > 0.f)
    {
        OwnerAnim->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
    }
}

void UWeaponComponent::StartCharged()
{
    if (!OwnerAnim || !ChargedAttackMontage) return;

    bIsAttacking = true;
    bHasLoopedChargedAttack = false;

    const float Len = OwnerAnim->Montage_Play(ChargedAttackMontage, Stats.AttackPlayRate, EMontagePlayReturnType::MontageLength, 0.f, true);
    if (Len > 0.f)
    {
        if (!ChargeLoopSection.IsNone())
        {
            OwnerAnim->Montage_JumpToSection(ChargeLoopSection, ChargedAttackMontage);
        }
        OwnerAnim->Montage_SetEndDelegate(OnAttackMontageEnded, ChargedAttackMontage);
    }
}

void UWeaponComponent::AttackMontageEnded(UAnimMontage* /*Montage*/, bool /*bInterrupted*/)
{
    bIsAttacking = false;

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
    {
        if (bIsChargingAttack)
        {
            StartCharged();
        }
        else
        {
            StartCombo();
        }
    }
}

// *** CHANGED [Notify Binding]: routes montage notifies to our AN_* functions
void UWeaponComponent::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& /*Payload*/)
{
    if (NotifyName == "CheckComboWindow")
    {
        AN_CheckComboWindow();
    }
    else if (NotifyName == "CheckCharged")
    {
        AN_CheckChargedAttack();
    }
    else if (NotifyName == "DoLightTrace")
    {
        AN_DoLightTrace(NAME_None);
    }
    else if (NotifyName == "DoHeavyTrace")
    {
        AN_DoHeavyTrace(NAME_None);
    }
}

// -------------------- Anim Notifies --------------------

void UWeaponComponent::AN_CheckComboWindow()
{
    if (!bIsAttacking || bIsChargingAttack || !OwnerAnim || !ComboAttackMontage) return;

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - CachedAttackInputTime > ComboInputCacheTimeTolerance) return;

    CachedAttackInputTime = -1000.0;

    const int32 Next = ComboIndex + 1;
    if (ComboSectionNames.IsValidIndex(Next))
    {
        OwnerAnim->Montage_JumpToSection(ComboSectionNames[Next], ComboAttackMontage);
        ComboIndex = Next;
    }
}

void UWeaponComponent::AN_CheckChargedAttack()
{
    bHasLoopedChargedAttack = true;

    if (!OwnerAnim || !ChargedAttackMontage) return;

    if (bIsChargingAttack)
    {
        if (!ChargeLoopSection.IsNone())
        {
            OwnerAnim->Montage_JumpToSection(ChargeLoopSection, ChargedAttackMontage);
        }
    }
    else
    {
        bHasLoopedChargedAttack = false;
        if (!ChargeAttackSection.IsNone())
        {
            OwnerAnim->Montage_JumpToSection(ChargeAttackSection, ChargedAttackMontage);
        }
    }
}

void UWeaponComponent::AN_DoLightTrace(FName DamageSourceBone)
{
    PerformMeleeTrace(LightTraceSpec, DamageSourceBone.IsNone() ? LightTraceSpec.SocketOrBone : DamageSourceBone);
}

void UWeaponComponent::AN_DoHeavyTrace(FName DamageSourceBone)
{
    PerformMeleeTrace(HeavyTraceSpec, DamageSourceBone.IsNone() ? HeavyTraceSpec.SocketOrBone : DamageSourceBone);
}

// -------------------- Trace helper --------------------

void UWeaponComponent::PerformMeleeTrace(const FWeaponMeleeTraceSpec& Spec, FName DamageSourceBone)
{
    if (!OwnerChar || !OwnerMesh) return;

    const FVector Start = Spec.bUseSocket
        ? OwnerMesh->GetSocketLocation(DamageSourceBone)
        : OwnerMesh->GetBoneLocation(DamageSourceBone);

    const FVector End = Start + OwnerChar->GetActorForwardVector() * Spec.Distance;

    TArray<FHitResult> Hits;

    FCollisionObjectQueryParams Obj;
    Obj.AddObjectTypesToQuery(ECC_Pawn);
    Obj.AddObjectTypesToQuery(ECC_WorldDynamic);

    FCollisionShape Shape = FCollisionShape::MakeSphere(Spec.Radius);

    // *** CHANGED: return initial overlaps so “start inside capsule” still registers
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WeaponMelee), false);
    Params.AddIgnoredActor(OwnerChar);
    Params.bFindInitialOverlaps = true;

    const bool bHit = GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, Shape, Params);

    if (/*Spec.bDebug*/true)
    {
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Start, Spec.Radius, 12, FLinearColor::Blue, 1.f, 1.f);
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), End, Spec.Radius, 12, FLinearColor::Green, 1.f, 1.f);
        UKismetSystemLibrary::DrawDebugLine(GetWorld(), Start, End, FLinearColor::White, 1.f, 1.f);
    }

    if (!bHit) return;

    // *** CHANGED: useful one-time log while debugging
    UE_LOG(LogTemp, Verbose, TEXT("MeleeTrace hits: %d"), Hits.Num());

    for (const FHitResult& Hit : Hits)
    {
        AActor* Target = Hit.GetActor();
        if (!Target || Target == OwnerChar) continue;

        const FVector ImpulseDir = (-Hit.ImpactNormal).GetSafeNormal();

        // *** CHANGED: prefer a Defence component if present
        if (UCombatDefenceComponent* Def = Target->FindComponentByClass<UCombatDefenceComponent>())
        {
            const FDamageResult R = Def->HandleIncomingAttack(OwnerChar, Stats.BaseDamage, Hit.ImpactPoint, ImpulseDir);
            if (R.ActualDamage > 0.f)
            {
                ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, R.ActualDamage, Hit.ImpactPoint);
            }
            continue;
        }

        // Interface on the actor (ProtagonistCharacter implements it)
        if (Target->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
        {
            const FDamageResult R =
                ICombatInterface::Execute_HandleIncomingAttack(Target, OwnerChar, Stats.BaseDamage, Hit.ImpactPoint, ImpulseDir);

            if (R.ActualDamage > 0.f)
            {
                ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, R.ActualDamage, Hit.ImpactPoint);
            }
            continue;
        }

        // *** CHANGED: real fallback for non-interface actors
        UGameplayStatics::ApplyPointDamage(
            Target, Stats.BaseDamage, ImpulseDir, Hit,
            OwnerChar ? OwnerChar->GetController() : nullptr,
            OwnerChar, UDamageType::StaticClass());
    }
}
