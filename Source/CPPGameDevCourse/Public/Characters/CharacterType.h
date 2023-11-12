#pragma once

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	ECS_Unequipped UMETA(DisplayName = "Unequipped"),
	ECS_EquippedOneHandedWeapon UMETA(DisplayName = "EquippedOneHandedWeapon"),
	ECS_EquippedTwoHandedWeapon UMETA(DisplayName = "EquippedTwoHandedWeapon")
};

UENUM(BlueprintType)
enum class EActionState : uint8
{
	EAS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	EAS_HitReact UMETA(DisplayName = "HitReact"),
	EAS_Occupied UMETA(DisplayName = "Ocuppied"),
	EAS_Attacking UMETA(DisplayName = "Attacking"),
	EAS_Dead UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class EEnemyDeathPose : uint8
{
	EEDP_Death5  UMETA(DisplayName = "Death5"),
	EEDP_Death4  UMETA(DisplayName = "Death4"),
	EEDP_Death3  UMETA(DisplayName = "Death3"),
	EEDP_Death2  UMETA(DisplayName = "Death2"),
	EEDP_Death1  UMETA(DisplayName = "Death1")
};

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	EES_NoState UMETA(DisplayName = "NoState"),
	EES_Dead UMETA(DisplayName = "Dead"),
	EES_Patrolling UMETA(DisplayName = "Patrolling"),
	EES_Chasing UMETA(DisplayName = "Chasing"),
	EES_Attacking UMETA(DisplayName = "Attacking"),
	EES_Engaged UMETA(DisplayName = "Engaged")
};