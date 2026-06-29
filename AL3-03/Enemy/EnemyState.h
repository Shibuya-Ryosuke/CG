#pragma once

class Enemy; // 前方宣言

// 状態の基底インターフェース
class IEnemyState {
public:
	virtual ~IEnemyState() = default;
	virtual void Update(Enemy* enemy) = 0;
};

// 接近状態
class EnemyStateApproach : public IEnemyState {
public:
	static EnemyStateApproach* GetInstance();
	void Update(Enemy* enemy) override;
private:
	EnemyStateApproach() = default; // 外部からのインスタンス化を禁止
};

// 離脱状態
class EnemyStateLeave : public IEnemyState {
public:
	static EnemyStateLeave* GetInstance();
	void Update(Enemy* enemy) override;
private:
	EnemyStateLeave() = default; // 外部からのインスタンス化を禁止
};