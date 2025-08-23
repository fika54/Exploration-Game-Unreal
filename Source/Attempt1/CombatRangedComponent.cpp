#include "CombatRangedComponent.h"
#include "CombatInterface.h"
#include "CombatProjectile.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

void UCombatRangedComponent::PrimaryAttack()
{
    Fire(PrimarySpec);
}

void UCombatRangedComponent::SecondaryAttack()
{
    Fire(SecondarySpec);
}

void UCombatRangedComponent::Fire(const FRangedAttackSpec& Spec)
{
    if (!OwnerChar) return;
    if (Spec.bUseHitscan) Hitscan(Spec);
    else                  SpawnProjectile(Spec);
}

bool UCombatRangedComponent::Hitscan(const FRangedAttackSpec& Spec)
{
    // Camera viewpoint
    FVector EyeLoc; FRotator EyeRot;
    if (OwnerChar->Controller) { OwnerChar->Controller->GetPlayerViewPoint(EyeLoc, EyeRot); }
    else { EyeLoc = OwnerChar->GetActorLocation(); EyeRot = OwnerChar->GetActorRotation(); }

    // Spread
    FRotator ShotRot = EyeRot;
    ShotRot.Yaw += FMath::RandRange(-Spec.SpreadDegrees, Spec.SpreadDegrees);
    ShotRot.Pitch += FMath::RandRange(-Spec.SpreadDegrees, Spec.SpreadDegrees);
    const FVector ShotDir = ShotRot.Vector();

    const FVector CamStart = EyeLoc;
    const FVector CamEnd = CamStart + ShotDir * Spec.Range;

    FHitResult CamHit;
    FCollisionQueryParams CamParams(SCENE_QUERY_STAT(RangedCamTrace), false, OwnerChar);
    CamParams.AddIgnoredActor(OwnerChar);
    const bool bCamHit = GetWorld()->LineTraceSingleByChannel(CamHit, CamStart, CamEnd, ECC_Visibility, CamParams);

    // Muzzle trace towards aim point
    FVector MuzzleLoc = OwnerChar->GetActorLocation();
    if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
    {
        MuzzleLoc = Mesh->GetSocketLocation(Spec.MuzzleSocket);
    }
    const FVector AimPoint = bCamHit ? CamHit.ImpactPoint : CamEnd;
    const FVector MuzzleDir = (AimPoint - MuzzleLoc).GetSafeNormal();
    const FVector MuzzleEnd = MuzzleLoc + MuzzleDir * Spec.Range;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(RangedMuzzleTrace), false, OwnerChar);
    Params.AddIgnoredActor(OwnerChar);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, MuzzleLoc, MuzzleEnd, ECC_Visibility, Params);

    if (Spec.bDebug)
    {
        UKismetSystemLibrary::DrawDebugLine(GetWorld(), MuzzleLoc, bHit ? Hit.ImpactPoint : MuzzleEnd, FLinearColor::Yellow, 1.f, 1.f);
        if (bHit) UKismetSystemLibrary::DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 12.f, FLinearColor::Red, 1.f);
    }

    if (!bHit || !Hit.GetActor()) return bHit;

    const FVector ImpulseDir = (-Hit.ImpactNormal).GetSafeNormal();
    AActor* Target = Hit.GetActor();

    if (Target->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
    {
        const FDamageResult Res = ICombatInterface::Execute_HandleIncomingAttack(Target, OwnerChar, Spec.Damage, Hit.ImpactPoint, ImpulseDir);
        if (Res.ActualDamage > 0.f)
        {
            OnDealtDamage.Broadcast(Res.ActualDamage, Hit.ImpactPoint);
            ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, Res.ActualDamage, Hit.ImpactPoint);
        }
    }
    else
    {
        ICombatInterface::Execute_ApplyRawDamage(Target, Spec.Damage, OwnerChar, Hit.ImpactPoint, ImpulseDir);
        OnDealtDamage.Broadcast(Spec.Damage, Hit.ImpactPoint);
        ICombatInterface::Execute_OnDealtDamage(OwnerChar, Target, Spec.Damage, Hit.ImpactPoint);
    }

    return true;
}

void UCombatRangedComponent::SpawnProjectile(const FRangedAttackSpec& Spec)
{
    if (!OwnerChar || !Spec.ProjectileClass) return;

    FVector MuzzleLoc = OwnerChar->GetActorLocation();
    FRotator MuzzleRot = OwnerChar->GetActorRotation();
    if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
    {
        const FTransform T = Mesh->GetSocketTransform(Spec.MuzzleSocket);
        MuzzleLoc = T.GetLocation();
        MuzzleRot = T.Rotator();
    }

    FActorSpawnParameters SP;
    SP.Owner = OwnerChar;
    SP.Instigator = OwnerChar;

    ACombatProjectile* P = GetWorld()->SpawnActor<ACombatProjectile>(Spec.ProjectileClass, MuzzleLoc, MuzzleRot, SP);
    if (P)
    {
        P->InitProjectile(OwnerChar, Spec.Damage, Spec.Range);
    }
}
