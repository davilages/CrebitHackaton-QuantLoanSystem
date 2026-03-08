// frontend/src/components/SimulationResults.jsx
import React from 'react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { TrendingUp, ShieldCheck, AlertTriangle, CheckCircle } from 'lucide-react';

export default function SimulationResults({ results }) {
  if (!results) return null;

  const { creditAvailable, riskLevel, confidence, simulationData, recommendations } = results;

  const getRiskColor = (level) => {
    switch (level) {
      case 'Baixo': return 'text-green-600 bg-green-100';
      case 'Médio': return 'text-yellow-600 bg-yellow-100';
      case 'Alto': return 'text-red-600 bg-red-100';
      default: return 'text-slate-600 bg-slate-100';
    }
  };

  return (
    <div className="space-y-6">
      {/* HEADER COM RESULTADO PRINCIPAL */}
      <div className="bg-gradient-to-r from-indigo-600 to-purple-600 text-white p-6 rounded-3xl shadow-xl">
        <div className="flex justify-between items-start mb-4">
          <div>
            <p className="text-indigo-200 text-sm font-bold uppercase">Crédito Disponível</p>
            <h2 className="text-4xl font-black mt-1">R$ {creditAvailable.toLocaleString()}</h2>
          </div>
          <div className="text-right">
            <div className={`inline-flex items-center gap-1 px-3 py-1 rounded-full text-xs font-bold ${getRiskColor(riskLevel)}`}>
              <ShieldCheck size={12} />
              Risco {riskLevel}
            </div>
            <p className="text-xs text-indigo-200 mt-1">{confidence}% de confiança</p>
          </div>
        </div>
        <p className="text-sm text-indigo-100 leading-relaxed">
          Baseado na sua previsibilidade de demanda para os próximos 7 dias usando Monte Carlo.
        </p>
        <button className="w-full mt-6 bg-white text-indigo-700 font-bold py-3 rounded-xl hover:bg-indigo-50 transition">
          Resgatar Agora
        </button>
      </div>

      {/* GRÁFICO DE SIMULAÇÃO */}
      <div className="bg-white p-6 rounded-2xl border border-slate-200">
        <div className="flex justify-between items-start mb-6">
          <div>
            <h3 className="font-bold text-slate-800 flex items-center gap-2">
              <TrendingUp size={20} />
              Simulação de Ganhos
            </h3>
            <p className="text-xs text-slate-400 italic">Modelo Monte Carlo: 10k iterações</p>
          </div>
          <div className="flex gap-4">
            <div className="flex items-center gap-1 text-[10px] text-slate-500 uppercase">
              <span className="w-3 h-3 bg-indigo-100 rounded-sm" /> Zona de Risco
            </div>
            <div className="flex items-center gap-1 text-[10px] text-slate-500 uppercase">
              <span className="w-3 h-3 bg-indigo-500 rounded-sm" /> Projeção Média
            </div>
          </div>
        </div>

        <div className="h-80 w-full">
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={simulationData}>
              <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#f1f5f9" />
              <XAxis
                dataKey="name"
                axisLine={false}
                tickLine={false}
                tick={{fontSize: 12, fill: '#94a3b8'}}
              />
              <YAxis
                axisLine={false}
                tickLine={false}
                tick={{fontSize: 12, fill: '#94a3b8'}}
                tickFormatter={(value) => `R$${value}`}
              />
              <Tooltip
                formatter={(value) => [`R$ ${value.toFixed(0)}`, '']}
                labelStyle={{color: '#374151'}}
              />
              {/* Zona de incerteza (Min até Max) */}
              <Area
                type="monotone"
                dataKey="max"
                stroke="none"
                fill="#e0e7ff"
                fillOpacity={0.6}
              />
              <Area
                type="monotone"
                dataKey="min"
                stroke="none"
                fill="#f8fafc"
                fillOpacity={1}
              />
              {/* Linha principal (Média/Esperado) */}
              <Area
                type="monotone"
                dataKey="avg"
                stroke="#6366f1"
                strokeWidth={3}
                fill="none"
              />
            </AreaChart>
          </ResponsiveContainer>
        </div>
      </div>

      {/* RECOMENDAÇÕES */}
      {recommendations && recommendations.length > 0 && (
        <div className="bg-white p-6 rounded-2xl border border-slate-200">
          <h3 className="flex items-center gap-2 font-bold text-slate-800 mb-4">
            <CheckCircle className="text-green-500" size={20} />
            Recomendações Personalizadas
          </h3>
          <ul className="space-y-3">
            {recommendations.map((rec, index) => (
              <li key={index} className="flex gap-3 text-sm text-slate-600">
                <div className="w-1.5 h-1.5 rounded-full bg-indigo-500 mt-1.5 shrink-0" />
                {rec}
              </li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
}