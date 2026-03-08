// Mock data for client simulation and Monte Carlo results

export interface Client {
  id: string;
  name: string;
  occupation: string;
  location: string;
  incomeFixed: number;
  incomeVariable: number;
  bankConnected: boolean;
}

export interface MonteCarloRequest {
  ss: number;
  loan_amount: number;
  pay_day: string;
}

export interface MonteCarloStatistics {
  avg_profit: number;
  repayment_probability: number;
  profit_std_dev: number;
  confidence_interval: {
    lower: number;
    upper: number;
  };
}

export interface InterestRateSweep {
  interest_rate: number;
  avg_profit: number;
  repayment_probability: number;
}

export interface MonteCarloResponse {
  request: MonteCarloRequest;
  statistics: MonteCarloStatistics;
  interest_rate_sweep: InterestRateSweep[];
  recommended_rate: number;
}

export interface SimulationDataPoint {
  name: string;
  min: number;
  max: number;
  avg: number;
}

export const mockClients: Client[] = [
  {
    id: "1",
    name: "Carlos Silva",
    occupation: "Uber Driver",
    location: "São Paulo, SP",
    incomeFixed: 0,
    incomeVariable: 100,
    bankConnected: true
  },
  {
    id: "2",
    name: "Ana Santos",
    occupation: "iFood Delivery Driver",
    location: "Rio de Janeiro, RJ",
    incomeFixed: 20,
    incomeVariable: 80,
    bankConnected: true
  },
  {
    id: "3",
    name: "João Oliveira",
    occupation: "Freelance Designer",
    location: "Belo Horizonte, MG",
    incomeFixed: 40,
    incomeVariable: 60,
    bankConnected: false
  },
  {
    id: "4",
    name: "Maria Costa",
    occupation: "Freelance Developer",
    location: "Curitiba, PR",
    incomeFixed: 30,
    incomeVariable: 70,
    bankConnected: true
  },
  {
    id: "5",
    name: "Pedro Ferreira",
    occupation: "99 Driver",
    location: "Brasília, DF",
    incomeFixed: 10,
    incomeVariable: 90,
    bankConnected: true
  },
  {
    id: "6",
    name: "Julia Almeida",
    occupation: "Freelance Photographer",
    location: "Porto Alegre, RS",
    incomeFixed: 25,
    incomeVariable: 75,
    bankConnected: false
  }
];

export const generateMonteCarloData = (clientId: string, loanAmount: number, payDay: string): MonteCarloResponse => {
  const client = mockClients.find(c => c.id === clientId);
  // Default repayment probability - in real app this would come from backend analysis
  const repaymentProbability = 75; // Default 75% repayment probability
  const baseRate = (100 - repaymentProbability) / 10; // Base rate calculation
  
  return {
    request: {
      ss: parseFloat(clientId),
      loan_amount: loanAmount,
      pay_day: payDay
    },
    statistics: {
      avg_profit: loanAmount * 1.15,
      repayment_probability: repaymentProbability,
      profit_std_dev: loanAmount * 0.08,
      confidence_interval: {
        lower: loanAmount * 0.95,
        upper: loanAmount * 1.35
      }
    },
    interest_rate_sweep: [
      { interest_rate: baseRate - 1, avg_profit: loanAmount * 1.10, repayment_probability: repaymentProbability + 8 },
      { interest_rate: baseRate, avg_profit: loanAmount * 1.15, repayment_probability: repaymentProbability },
      { interest_rate: baseRate + 1, avg_profit: loanAmount * 1.20, repayment_probability: repaymentProbability - 5 },
      { interest_rate: baseRate + 2, avg_profit: loanAmount * 1.25, repayment_probability: repaymentProbability - 12 },
      { interest_rate: baseRate + 3, avg_profit: loanAmount * 1.30, repayment_probability: repaymentProbability - 18 }
    ],
    recommended_rate: baseRate
  };
};

export const generateSimulationData = (loanAmount: number): SimulationDataPoint[] => {
  return [
    { name: "Week 1", min: loanAmount * 0.85, max: loanAmount * 1.15, avg: loanAmount * 1.00 },
    { name: "Week 2", min: loanAmount * 0.90, max: loanAmount * 1.20, avg: loanAmount * 1.05 },
    { name: "Week 3", min: loanAmount * 0.88, max: loanAmount * 1.25, avg: loanAmount * 1.08 },
    { name: "Week 4", min: loanAmount * 0.92, max: loanAmount * 1.28, avg: loanAmount * 1.12 },
    { name: "Week 5", min: loanAmount * 0.95, max: loanAmount * 1.30, avg: loanAmount * 1.15 },
    { name: "Week 6", min: loanAmount * 0.93, max: loanAmount * 1.32, avg: loanAmount * 1.18 },
    { name: "Week 7", min: loanAmount * 0.96, max: loanAmount * 1.33, avg: loanAmount * 1.20 },
    { name: "Week 8", min: loanAmount * 0.94, max: loanAmount * 1.35, avg: loanAmount * 1.22 }
  ];
};
