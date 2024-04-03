#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_Equipped UMETA(DisplayName = "Equipped"),
	ECS_SuperArmor UMETA(DisplayName = "SuperArmor"),
	ECS_Attacking UMETA(DisplayName = "Attacking"),
	ECS_UnderAttack UMETA(DisplayName = "UnderAttack"),

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};