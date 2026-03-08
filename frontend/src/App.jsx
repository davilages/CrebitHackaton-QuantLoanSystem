import React, { useState } from 'react';
import { MapPin, TrendingUp, AlertTriangle, ShieldCheck, Info } from 'lucide-react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import UserForm from './components/UserForm';
import SimulationResults from './components/SimulationResults';
import { apiService } from './services/apiService';

export default function App() {
  const [simulationResults, setSimulationResults] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [userData, setUserData] = useState(null);

  const handleFormSubmit = async (formData) => {
    setIsLoading(true);
    setUserData(formData);

    try {
      // Chama a API (por enquanto simulada)
      const results = await apiService.simulateCredit(formData);
      setSimulationResults(results);
    } catch (error) {
      console.error('Erro na simulação:', error);
      // TODO: Mostrar erro para o usuário
    } finally {
      setIsLoading(false);
    }
  };

  const handleReset = () => {
    setSimulationResults(null);
    setUserData(null);
  };

  return (
    <div className="min-h-screen bg-slate-50 text-slate-900 font-sans p-4 md:p-8">
      <div className="max-w-6xl mx-auto space-y-6">

        {/* HEADER */}
        <div className="flex justify-between items-center bg-white p-6 rounded-2xl shadow-sm border border-slate-200">
          <div>
            <h1 className="text-2xl font-black text-indigo-600 italic">GIG-FLOW</h1>
            <p className="text-slate-500 text-sm italic">O score de quem faz a cidade girar.</p>
          </div>
          <div className="text-right">
            <p className="font-bold text-slate-800">{userData?.name || 'Usuário'}</p>
            <div className="flex items-center gap-1 text-xs text-slate-400">
              <MapPin size={12} /> {userData?.location === 'tampa-fl' ? 'Tampa, FL' : userData?.location || 'Localização'}
            </div>
          </div>
          {simulationResults && (
            <button
              onClick={handleReset}
              className="text-sm text-indigo-600 hover:text-indigo-800 underline"
            >
              Novo Cálculo
            </button>
          )}
        </div>

        {/* CONTEÚDO PRINCIPAL */}
        {!simulationResults ? (
          // MOSTRA FORMULÁRIO QUANDO NÃO TEM RESULTADOS
          <div className="max-w-2xl mx-auto">
            <UserForm onSubmit={handleFormSubmit} isLoading={isLoading} />
          </div>
        ) : (
          // MOSTRA RESULTADOS QUANDO TEM
          <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
            {/* COLUNA ESQUERDA: RESULTADOS */}
            <div className="lg:col-span-1">
              <SimulationResults results={simulationResults} />
            </div>

            {/* COLUNA DIREITA: GRÁFICO DETALHADO */}
            <div className="lg:col-span-2 bg-white p-6 rounded-2xl border border-slate-200 flex flex-col">
              <div className="flex justify-between items-start mb-6">
                <div>
                  <h3 className="font-bold text-slate-800">Análise Detalhada</h3>
                  <p className="text-xs text-slate-400 italic">Distribuição de probabilidade dos ganhos</p>
                </div>
                <div className="flex gap-4">
                  <div className="flex items-center gap-1 text-[10px] text-slate-500 uppercase">
                    <span className="w-3 h-3 bg-indigo-100 rounded-sm" /> Risco
                  </div>
                  <div className="flex items-center gap-1 text-[10px] text-slate-500 uppercase">
                    <span className="w-3 h-3 bg-indigo-500 rounded-sm" /> Esperado
                  </div>
                </div>
              </div>

              <div className="flex-1 min-h-[300px] w-full">
                <ResponsiveContainer width="100%" height="100%">
                  <AreaChart data={simulationResults.simulationData}>
                    <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#f1f5f9" />
                    <XAxis dataKey="name" axisLine={false} tickLine={false} tick={{fontSize: 12, fill: '#94a3b8'}} />
                    <YAxis axisLine={false} tickLine={false} tick={{fontSize: 12, fill: '#94a3b8'}} />
                    <Tooltip />
                    {/* O LEQUE DE INCERTEZA (Min até Max) */}
                    <Area type="monotone" dataKey="max" stroke="none" fill="#e0e7ff" fillOpacity={0.6} />
                    <Area type="monotone" dataKey="min" stroke="none" fill="#f8fafc" fillOpacity={1} />
                    {/* A LINHA PRINCIPAL (Média/Esperado) */}
                    <Area type="monotone" dataKey="avg" stroke="#6366f1" strokeWidth={3} fill="none" />
                  </AreaChart>
                </ResponsiveContainer>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}