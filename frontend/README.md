# GIG-FLOW Frontend

Sistema de crédito para trabalhadores da gig economy usando simulação Monte Carlo.

## 🚀 Como executar

```bash
cd frontend
npm install
npm run dev
```

Acesse: http://localhost:5174/

## 📋 Funcionalidades

### Formulário de Perfil
- **Nome**: Identificação do usuário
- **Profissão**: Uber Driver, iFood, Uber Eats, Freelancer, Estudante
- **Renda Mensal**: Estimativa de ganhos
- **Despesas Mensais**: Custos operacionais
- **Tolerância ao Risco**: 1-10 (Conservador → Agressivo)
- **Localização**: Cidade de atuação

### Resultados da Simulação
- **Crédito Disponível**: Valor calculado via Monte Carlo
- **Nível de Risco**: Baixo/Médio/Alto
- **Confiança**: Percentual de acurácia
- **Gráfico de Ganhos**: Projeção semanal com zona de incerteza
- **Recomendações**: Sugestões personalizadas

## 🔧 Tecnologias

- **React 19** - Framework frontend
- **Tailwind CSS** - Estilização
- **Recharts** - Gráficos
- **Lucide React** - Ícones
- **Vite** - Build tool

## 🔌 Integração com Backend

O frontend se conecta com o backend C++ via API REST:

```javascript
// Simulação atual (mock)
const results = await apiService.simulateCredit(userData);

// Quando o backend estiver pronto:
POST /api/simulate
{
  "monthlyIncome": 3000,
  "monthlyExpenses": 1500,
  "profession": "uber-driver",
  "riskTolerance": 7,
  "location": "tampa-fl"
}
```

## 📊 Algoritmo Monte Carlo

1. **GBM (Geometric Brownian Motion)**: Simula preços/ativos
2. **10.000 iterações**: Cada cenário de 7 dias
3. **VaR (Value at Risk)**: Calcula risco de perda
4. **Árvore de Decisão**: 3 cenários (Agressivo/Moderado/Conservador)

## 🎯 Público-Alvo

- Motoristas Uber/Uber Eats
- Entregadores iFood/Rappi
- Freelancers
- Estudantes
- Autônomos sem holerite fixo

## 📈 Métricas de Sucesso

- **Score de Previsibilidade**: Baseado em demanda futura
- **Zona de Risco**: Área entre min/max nos gráficos
- **Confiança Estatística**: % baseado em histórico

---

**Hackathon Quant Loan System** - Crédito justo para quem faz a cidade girar! 🚗💨