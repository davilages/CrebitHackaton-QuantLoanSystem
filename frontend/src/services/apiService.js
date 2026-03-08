// frontend/src/services/apiService.js
const API_BASE_URL = 'http://localhost:3001/api'; // FIX ME: when backend is ready

export const apiService = {
  // Simula a chamada para o backend (por enquanto usa dados mockados)
  async simulateCredit(userData) {
    // Simula delay de rede
    await new Promise(resolve => setTimeout(resolve, 2000));

    // Dados que viriam do backend (Monte Carlo Simulation)
    const simulationResults = {
      creditAvailable: Math.floor(Math.random() * 500) + 200, // $200-$700
      riskLevel: userData.riskTolerance < 4 ? 'Baixo' : userData.riskTolerance < 7 ? 'Médio' : 'Alto',
      confidence: Math.floor(Math.random() * 30) + 70, // 70-100%
      simulationData: generateSimulationData(userData),
      recommendations: generateRecommendations(userData)
    };

    return simulationResults;
  }
};

// Gera dados simulados baseados no perfil do usuário
function generateSimulationData(userData) {
  const days = ['Seg', 'Ter', 'Qua', 'Qui', 'Sex', 'Sab', 'Dom'];
  const baseIncome = userData.monthlyIncome / 30; // Renda diária média

  return days.map((day, index) => {
    // Simula variação baseada no dia da semana e perfil
    const dayMultiplier = [0.8, 0.9, 1.0, 1.2, 1.5, 1.3, 0.7][index]; // Padrão semanal
    const volatility = userData.riskTolerance / 10; // Quanto mais risco, mais volatilidade

    const avg = baseIncome * dayMultiplier;
    const variation = avg * volatility;

    return {
      name: day,
      min: Math.max(0, avg - variation),
      avg: avg,
      max: avg + variation * 2
    };
  });
}

// Gera recomendações baseadas no perfil
function generateRecommendations(userData) {
  const recommendations = [];

  if (userData.profession === 'uber-driver') {
    recommendations.push('Alta demanda detectada: Eventos esportivos próximos');
  }

  if (userData.monthlyExpenses > userData.monthlyIncome * 0.7) {
    recommendations.push('Considere reduzir despesas para melhorar score');
  }

  if (userData.riskTolerance < 5) {
    recommendations.push('Perfil conservador: Crédito menor mas mais seguro');
  }

  return recommendations;
}