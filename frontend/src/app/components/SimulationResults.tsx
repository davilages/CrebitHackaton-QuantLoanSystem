import React from 'react';
import InterestCard from './results/InterestCard';
import { SensitivityAnalysis } from './results/SensitivityAnalysis';
import MonteCarloChart from './results/MonteCarloChart';
import ExplainableAI from './results/ExplainableAI';
import { StatsGrid } from './results/StatsGrid';
import { RiskCard } from './results/RiskCard';

interface SimulationResultsProps {
  results: any; // TODO: Define more specific interface
}

export default function SimulationResults({ results }: SimulationResultsProps): React.JSX.Element {
    if (!results) return <></>;

    const { response } = results;
    const { statistics, interest_rate_sweep, recommended_interest_rate, request } = response;

	return (
		<div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
			{/* Coluna da Esquerda: Cards e IA */}
			<div className="lg:col-span-1 space-y-6">
				<InterestCard
					rate={recommended_interest_rate}
					probability={1 - (statistics?.repayment_probability || 0)}
				/>

				{/* Novo StatsGrid */}
				<StatsGrid
					avgProfit={statistics?.avg_profit || 0}
					stdDev={statistics?.profit_std_dev || 0}
					confidenceLower={statistics?.confidence_interval?.lower || 0}
					confidenceUpper={statistics?.confidence_interval?.upper || 0}
				/>

				{/* Novo RiskCard */}
				<RiskCard
					riskScore={Math.round((1 - (statistics?.repayment_probability || 0)) * 100)}
					occupation={request?.occupation || "Not provided"}
					location={request?.location || "Not provided"}
				/>

				<ExplainableAI
					statistics={statistics}
					payDay={request?.pay_day}
				/>
			</div>

			{/* Right Column: Charts and Sweep */}
			<div className="lg:col-span-2 space-y-6">
				<MonteCarloChart data={results.simulationData} />
				<SensitivityAnalysis
					sweepData={interest_rate_sweep}
					recommendedRate={recommended_interest_rate}
				/>
			</div>
		</div>
	);
}