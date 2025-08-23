#include "CombatProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CombatInterface.h"

ACombatProjectile::ACombatProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(6.f);
    Collision->SetCollisionProfileName(TEXT("Projectile"));
    Collision->OnComponentHit.AddDynamic(this, &ACombatProjectile::OnProjectileHit);
    SetRootComponent(Collision);

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->InitialSpeed = InitialSpeed;
    Movement->MaxSpeed = InitialSpeed;
    Movement->bRotationFollowsVelocity = true;
    Movement->ProjectileGravityScale = 0.1f;
}

void ACombatProjectile::InitProjectile(AActor* InShooter, float InDamage, float InRange)
{
    Shooter = InShooter;
    Damage = InDamage;

    if (Movement)
    {
        Movement->Velocity = GetActorForwardVector() * InitialSpeed;
    }

    const float Speed = FMath::Max(1.f, InitialSpeed);
    const float TimeByRange = InRange / Speed;
    SetLifeSpan(FMath::Min(LifeSeconds, TimeByRange));
}

void ACombatProjectile::BeginPlay()
{
    Super::BeginPlay();
}

void ACombatProjectile::OnProjectileHit(UPrimitiveComponent* /*HitComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/, const FHitResult& Hit)
{
    if (OtherActor && OtherActor != this && OtherActor != Shooter.Get())
    {
        const FVector ImpulseDir = (-Hit.ImpactNormal).GetSafeNormal();

        if (OtherActor->GetClass()->ImplementsInterface(UCombatInterface::StaticClass()))
        {
            ICombatInterface::Execute_HandleIncomingAttack(OtherActor, Shooter.Get(), Damage, Hit.ImpactPoint, ImpulseDir);
        }
        else
        {
            ICombatInterface::Execute_ApplyRawDamage(OtherActor, Damage, Shooter.Get(), Hit.ImpactPoint, ImpulseDir);
        }
    }

    Destroy();
}
