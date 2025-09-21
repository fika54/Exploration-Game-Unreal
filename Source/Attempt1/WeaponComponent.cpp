// WeaponComponent.cpp

#include "WeaponComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CombatInterface.h"

UWeaponComponent::UWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    Stats.BaseDamage = 1.0f;
    Stats.AttackPlayRate = 1.0f;
    Stats.AttackForce = 250.0f;

    LightTraceSpec.Distance = 75.0f;
    LightTraceSpec.Radius = 60.0f;
    LightTraceSpec.bUseSocket = true;
    LightTraceSpec.SocketOrBone = FName("hand_r");

    HeavyTraceSpec = LightTraceSpec;
    HeavyTraceSpec.Distance = 90.0f;
    HeavyTraceSpec.Radius = 75.0f;

    OnAttackMontageEnded.BindUObject(this, &UWeaponComponent::AttackMontageEnded);
}

void UWeaponComponent::BeginPlay()
{
    Super::BeginPlay();

    RefreshOwnerPointers();
    EnsureWeaponComponents();

    if (OwnerAnim)
    {
        OwnerAnim->OnPlayMontageNotifyBegin.AddDynamic(this, &UWeaponComponent::OnMontageNotifyBegin);
    }

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

// Owner anim layers and locomotion (optional)

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
            if (CachedRunSpeed > 0.0f)  Move->MaxWalkSpeed = CachedRunSpeed;
            if (CachedWalkSpeed > 0.0f) Move->MaxWalkSpeedCrouched = CachedWalkSpeed;
        }
    }
}

// Equip / Holster

void UWeaponComponent::EquipWeapon(bool bPlayMontage)
{
    RefreshOwnerPointers();
    EnsureWeaponComponents();

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

// Attacks (montage + section flow)

void UWeaponComponent::PrimaryAttack()
{
    if (!OwnerAnim || !ComboAttackMontage || ComboSectionNames.Num() == 0) return;

    const double Now = GetWorld()->GetTimeSeconds();
    if (bIsAttacking)
    {
        CachedAttackInputTime = Now;
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

    BeginHitWindow();

    const float Len = OwnerAnim->Montage_Play(
        ComboAttackMontage,
        Stats.AttackPlayRate,
        EMontagePlayReturnType::MontageLength,
        0.0f,
        true
    );
    if (Len > 0.0f)
    {
        OwnerAnim->Montage_SetEndDelegate(OnAttackMontageEnded, ComboAttackMontage);
    }
}

void UWeaponComponent::StartCharged()
{
    if (!OwnerAnim || !ChargedAttackMontage) return;

    bIsAttacking = true;
    bHasLoopedChargedAttack = false;

    BeginHitWindow();

    const float Len = OwnerAnim->Montage_Play(
        ChargedAttackMontage,
        Stats.AttackPlayRate,
        EMontagePlayReturnType::MontageLength,
        0.0f,
        true
    );
    if (Len > 0.0f)
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
    EndHitWindow();

    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - CachedAttackInputTime <= AttackInputCacheTimeTolerance)
    {
        if (bIsChargingAttack) StartCharged();
        else StartCombo();
    }
}

// Anim notifies

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

        BeginHitWindow();
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
            BeginHitWindow();
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

void UWeaponComponent::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& /*Payload*/)
{
    if (NotifyName == FName("HitWindowStart")) { BeginHitWindow(); }
    else if (NotifyName == FName("HitWindowEnd")) { EndHitWindow(); }
}

// Hit window helpers

void UWeaponComponent::BeginHitWindow()
{
    HitActorsCurrentWindow.Reset();
}

void UWeaponComponent::EndHitWindow()
{
    HitActorsCurrentWindow.Reset();
}

void UWeaponComponent::AN_HitWindowStart()
{
    BeginHitWindow();
}

void UWeaponComponent::AN_HitWindowEnd()
{
    EndHitWindow();
}

// Trace helper

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
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChar);

    const bool bHit = GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Obj, Shape, Params);

    if (/*Spec.bDebug*/true)
    {
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Start, Spec.Radius, 12, FLinearColor::Blue, 1.0f, 1.0f);
        UKismetSystemLibrary::DrawDebugSphere(GetWorld(), End, Spec.Radius, 12, FLinearColor::Green, 1.0f, 1.0f);
        UKismetSystemLibrary::DrawDebugLine(GetWorld(), Start, End, FLinearColor::White, 1.0f, 1.0f);
    }

    if (!bHit) return;

    int c = 0;

    for (const FHitResult& Hit : Hits)
    {
        
        AActor* Target = Hit.GetActor();
        if (!Target || Target == OwnerChar) continue;

        if (HitActorsCurrentWindow.Contains(Target)) continue;
        HitActorsCurrentWindow.Add(Target);

        

        FVector ToTarget2D = (Target->GetActorLocation() - OwnerChar->GetActorLocation());
        ToTarget2D.Z = 0.f;
        const FVector ImpulseDir = ToTarget2D.IsNearlyZero() ? OwnerChar->GetActorForwardVector() : ToTarget2D.GetSafeNormal();

        if (Target->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
        {
            const FDamageResult Result =
                ICombatInterface::Execute_HandleIncomingAttack(Target, OwnerChar, Stats.BaseDamage, Hit.ImpactPoint, ImpulseDir);
            
            if (Result.ActualDamage >= 0.0f)
            {
                UKismetSystemLibrary::DrawDebugSphere(GetWorld(), Start, Spec.Radius, 20 + HitActorsCurrentWindow.Num(), FLinearColor::Red, 1.0f, 1.0f);
                ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, Result.ActualDamage, Hit.ImpactPoint);
            }
        }
        else
        {
            ICombatInterface::Execute_ApplyRawDamage(Target, Stats.BaseDamage, OwnerChar, Hit.ImpactPoint, ImpulseDir);
            ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, Stats.BaseDamage, Hit.ImpactPoint);
        }
    }
}

void UWeaponComponent::onDodge()
{
    OwnerAnim->Montage_Play(
        DodgeMontage,
        Stats.AttackPlayRate,
        EMontagePlayReturnType::MontageLength,
        0.0f,
        true
    );
}
