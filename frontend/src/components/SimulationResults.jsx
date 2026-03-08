import React from 'react';
import InterestCard from './results/InterestCard';
import SensitivitySweep from './results/SensitivitySweep';
import MonteCarloChart from './results/MonteCarloChart';
import ExplainableAI from './results/ExplainableAI';

export default function SimulationResults({ results }) {
    if (!results) return null;

    const { response, simulationData } = results;
    const { statistics, interest_rate_sweep, recommended_interest_rate, request } = response;

    return (
        <div className="space-y-6">
            <InterestCard 
                rate={recommended_interest_rate} 
                probability={statistics?.repayment_probability} 
            />

            <SensitivitySweep data={interest_rate_sweep} />

            <div className="grid grid-cols-2 gap-4">
                <StatCard label="Desvio Padrão" value={statistics?.profit_std_dev} color="text-slate-700" />
                <StatCard label="Lucro Máximo" value={statistics?.max_profit} color="text-green-600" isMono />
            </div>

            <MonteCarloChart data={simulationData} />

            <ExplainableAI 
                statistics={statistics} 
                payDay={request?.pay_day} 
            />
        </div>
    );
}

const StatCard = ({ label, value, color, isMono }) => (
    <div className="bg-white p-4 rounded-2xl border border-slate-200 text-center">
        <p className="text-slate-400 text-[10px] uppercase font-bold">{label}</p>
        <p className={`text-lg font-bold ${color} ${isMono ? 'font-mono' : ''}`}>
            R$ {value || 0}
        </p>
    </div>
);