// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicMonster.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

ABasicMonster::ABasicMonster()
{
	PrimaryActorTick.bCanEverTick = true;

}

void ABasicMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABasicMonster, Hp);
}

void ABasicMonster::BeginPlay()
{
	Super::BeginPlay();
	
	//GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABasicMonster::OnAttacked);
	}
}

void ABasicMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABasicMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABasicMonster::WeaponHit_Implementation(FHitResult HitResult)
{
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), HitParticles, HitResult.Location, FRotator(0.f), true);
	}
}

void ABasicMonster::OnAttacked(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{ // server only
	Hp = FMath::Clamp(Hp - Damage, 0.f, MaxHp);
	UE_LOG(LogTemp, Warning, TEXT("Hp : %f"), Hp);
	if (FMath::IsNearlyZero(Hp))
	{ // Death
		// TODO : GameMode, SpearmanCharacter->GameState?
		Destroy();
	}
	else
	{ // HitReact
		// TODO : PlayHitReactMontage
	}
}

void ABasicMonster::OnRep_Hp()
{

}