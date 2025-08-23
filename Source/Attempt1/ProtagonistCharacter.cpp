#include "ProtagonistCharacter.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "CombatDefenceComponent.h"
#include "CombatMeleeComponent.h"
#include "CombatRangedComponent.h"
#include "CombatAttackComponent.h"

DEFINE_LOG_CATEGORY(LogProtagonistCharacter);

AProtagonistCharacter::AProtagonistCharacter()
{
    // Capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Controller rotation -> camera only
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Movement tuning
    UCharacterMovementComponent* Move = GetCharacterMovement();
    Move->bOrientRotationToMovement = true;
    Move->RotationRate = FRotator(0.f, 500.f, 0.f);
    Move->JumpZVelocity = 500.f;
    Move->AirControl = 0.35f;
    Move->MaxWalkSpeed = 500.f;
    Move->MinAnalogWalkSpeed = 20.f;
    Move->BrakingDecelerationWalking = 2000.f;
    Move->BrakingDecelerationFalling = 1500.f;

    // Camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->bUsePawnControlRotation = true;

    // Follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // Combat components
    Defence = CreateDefaultSubobject<UCombatDefenceComponent>(TEXT("Defence"));
    Melee = CreateDefaultSubobject<UCombatMeleeComponent>(TEXT("Melee"));
    Ranged = CreateDefaultSubobject<UCombatRangedComponent>(TEXT("Ranged"));

    ActiveAttack = Melee; // default
}

void AProtagonistCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (JumpAction)
        {
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        if (MoveAction)      EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Move);
        if (MouseLookAction) EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Look);
        if (LookAction)      EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProtagonistCharacter::Look);

        if (PrimaryAttackAction)   EIC->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnPrimary);
        if (SecondaryAttackAction) EIC->BindAction(SecondaryAttackAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnSecondary);

        if (BlockAction)
        {
            EIC->BindAction(BlockAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnBlockStart);
            EIC->BindAction(BlockAction, ETriggerEvent::Completed, this, &AProtagonistCharacter::OnBlockEnd);
        }

        if (DodgeAction) EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &AProtagonistCharacter::OnDodge);
    }
    else
    {
        UE_LOG(LogProtagonistCharacter, Error, TEXT("%s: EnhancedInputComponent not found."), *GetNameSafe(this));
    }
}

void AProtagonistCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D V = Value.Get<FVector2D>();
    DoMove(V.X, V.Y);
}

void AProtagonistCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D V = Value.Get<FVector2D>();
    DoLook(V.X, V.Y);
}

void AProtagonistCharacter::DoMove(float Right, float Forward)
{
    if (Controller)
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

        const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDir, Forward);
        AddMovementInput(RightDir, Right);
    }
}

void AProtagonistCharacter::DoLook(float Yaw, float Pitch)
{
    if (Controller)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void AProtagonistCharacter::DoJumpStart() { Jump(); }
void AProtagonistCharacter::DoJumpEnd() { StopJumping(); }

// Equip helpers
void AProtagonistCharacter::EquipMelee() { ActiveAttack = Melee; }
void AProtagonistCharacter::EquipRanged() { ActiveAttack = Ranged; }

// Combat input handlers
void AProtagonistCharacter::OnPrimary()
{
    if (ActiveAttack) ActiveAttack->PrimaryAttack();
}

void AProtagonistCharacter::OnSecondary()
{
    if (ActiveAttack) ActiveAttack->SecondaryAttack();
}

void AProtagonistCharacter::OnBlockStart() { StartBlock(); }
void AProtagonistCharacter::OnBlockEnd() { EndBlock(); }

void AProtagonistCharacter::OnDodge()
{
    FVector Dir = GetActorForwardVector();
    if (Controller)
    {
        const FRotator Rot = Controller->GetControlRotation();
        Dir = FRotationMatrix(FRotator(0.f, Rot.Yaw, 0.f)).GetUnitAxis(EAxis::X);
    }
    if (Defence) Defence->Dodge(Dir);
}

// AnimNotifies for melee hit frames
void AProtagonistCharacter::AN_MeleeLightTrace()
{
    if (Melee) Melee->PerformAttackTrace(Melee->LightTraceSpec);
}

void AProtagonistCharacter::AN_MeleeHeavyTrace()
{
    if (Melee) Melee->PerformAttackTrace(Melee->HeavyTraceSpec);
}

// ICombatInterface forwarding
void AProtagonistCharacter::TryAttack_Implementation(EAttackType Type)
{
    if (!ActiveAttack) return;
    if (Type == EAttackType::Light) ActiveAttack->PrimaryAttack();
    else                            ActiveAttack->SecondaryAttack();
}

void AProtagonistCharacter::PerformAttackTrace_Implementation(const FAttackTraceSpec& Spec)
{
    if (Melee) Melee->PerformAttackTrace(Spec);
}

void AProtagonistCharacter::StartBlock_Implementation()
{
    if (Defence) Defence->StartBlock();
}

void AProtagonistCharacter::EndBlock_Implementation()
{
    if (Defence) Defence->EndBlock();
}

float AProtagonistCharacter::GetParryWindowSeconds_Implementation() const
{
    return Defence ? Defence->GetParryWindow() : 0.25f;
}

FDamageResult AProtagonistCharacter::HandleIncomingAttack_Implementation(
    AActor* Attacker, float IncomingDamage, const FVector& ImpactPoint, const FVector& ImpulseDir)
{
    return Defence ? Defence->HandleIncomingAttack(Attacker, IncomingDamage, ImpactPoint, ImpulseDir)
        : FDamageResult{};
}

void AProtagonistCharacter::ApplyRawDamage_Implementation(
    float Damage, AActor* Causer, const FVector& ImpactPoint, const FVector& ImpulseDir)
{
    if (Defence) Defence->ApplyRawDamage(Damage, Causer, ImpactPoint, ImpulseDir);
}
