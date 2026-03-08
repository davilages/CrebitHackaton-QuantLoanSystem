from faker import Faker
import random
import json
from datetime import datetime, timedelta

fake = Faker()

PROFESSIONS = {
    "student": 1,
    "uber_driver": 2,
    "freelance_dev": 3,
    "ifood_delivery": 4
}

LOCATIONS = {
    "Florida": 1,
    "New York": 2,
    "California": 3,
    "Texas": 4
}

BALANCE_RANGE = {
    "student": (200, 1200),
    "uber_driver": (1200, 3500),
    "freelance_dev": (2500, 7000),
    "ifood_delivery": (800, 2500)
}

DAILY_CHANGE_RANGE = {
    "student": (-80, 60),
    "uber_driver": (-120, 180),
    "freelance_dev": (-200, 250),
    "ifood_delivery": (-100, 140)
}

FIXED_INCOME_RANGE = {
    "student": (300, 900),
    "uber_driver": (1800, 3200),
    "freelance_dev": (2500, 6000),
    "ifood_delivery": (1200, 2400)
}

MANAGER_SSN = 111111111


def generate_balances(days=30, starting_balance=1000, profession="student"):
    balances = []
    balance = starting_balance
    current_date = datetime.today() - timedelta(days=days)

    for _ in range(days):
        change = random.randint(*DAILY_CHANGE_RANGE[profession])
        balance += change

        if balance < 0:
            balance = 0

        balances.append({
            "date": current_date.strftime("%Y-%m-%d"),
            "balance": round(float(balance), 2)
        })

        current_date += timedelta(days=1)

    return balances


def generate_client():
    profession = random.choice(list(PROFESSIONS.keys()))
    job_id = PROFESSIONS[profession]

    location = random.choice(list(LOCATIONS.keys()))
    location_id = LOCATIONS[location]

    client_ssn = random.randint(100000000, 999999999)

    fixed_income_amount = round(float(random.randint(*FIXED_INCOME_RANGE[profession])), 2)
    pay_day = random.choice([1, 5, 10, 15, 20, 25, 28])

    start_date = datetime.today() - timedelta(days=60)
    starting_balance = random.randint(*BALANCE_RANGE[profession])

    balances = generate_balances(
        days=30,
        starting_balance=starting_balance,
        profession=profession
    )

    return {
        "manager_ssn": MANAGER_SSN,
        "client_ssn": client_ssn,
        "location_id": location_id,
        "job_id": job_id,
        "fixed_income": [
            {
                "pay_day": pay_day,
                "amount": fixed_income_amount,
                "start_date": start_date.strftime("%Y-%m-%d")
            }
        ],
        "balances": balances
    }


def generate_dataset(num_clients=20):
    return [generate_client() for _ in range(num_clients)]


def main():
    data = generate_dataset(num_clients=20)

    with open("clients_endpoint_data.json", "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4)

    print("Client endpoint dataset generated! File saved as clients_endpoint_data.json")


if __name__ == "__main__":
    main()