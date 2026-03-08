// frontend/src/components/UserForm.jsx
import React, { useState } from 'react';
import { User, DollarSign, Car, TrendingUp, AlertCircle } from 'lucide-react';

export default function UserForm({ onSubmit, isLoading }) {
  const [formData, setFormData] = useState({
    name: '',
    profession: 'uber-driver',
    monthlyIncome: '',
    monthlyExpenses: '',
    riskTolerance: 5,
    location: 'tampa-fl'
  });

  const handleSubmit = (e) => {
    e.preventDefault();
    onSubmit(formData);
  };

  const handleChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: name === 'monthlyIncome' || name === 'monthlyExpenses' || name === 'riskTolerance'
        ? Number(value)
        : value
    }));
  };

  return (
    <div className="bg-white p-6 rounded-2xl border border-slate-200 shadow-sm">
      <div className="flex items-center gap-2 mb-6">
        <User className="text-indigo-600" size={24} />
        <h2 className="text-xl font-bold text-slate-800">Seu Perfil</h2>
      </div>

      <form onSubmit={handleSubmit} className="space-y-6">
        {/* Nome */}
        <div>
          <label className="block text-sm font-medium text-slate-700 mb-2">
            Nome Completo
          </label>
          <input
            type="text"
            name="name"
            value={formData.name}
            onChange={handleChange}
            className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
            placeholder="João Silva"
            required
          />
        </div>

        {/* Profissão */}
        <div>
          <label className="block text-sm font-medium text-slate-700 mb-2">
            Profissão
          </label>
          <select
            name="profession"
            value={formData.profession}
            onChange={handleChange}
            className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
          >
            <option value="uber-driver">Motorista Uber</option>
            <option value="ifood-delivery">Entregador iFood</option>
            <option value="uber-eats">Entregador Uber Eats</option>
            <option value="freelancer">Freelancer</option>
            <option value="student">Estudante</option>
          </select>
        </div>

        {/* Renda Mensal */}
        <div>
          <label className="block text-sm font-medium text-slate-700 mb-2">
            Renda Mensal Estimada (R$)
          </label>
          <div className="relative">
            <DollarSign className="absolute left-3 top-3.5 text-slate-400" size={18} />
            <input
              type="number"
              name="monthlyIncome"
              value={formData.monthlyIncome}
              onChange={handleChange}
              className="w-full pl-10 pr-4 py-3 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              placeholder="3000"
              min="0"
              required
            />
          </div>
        </div>

        {/* Despesas Mensais */}
        <div>
          <label className="block text-sm font-medium text-slate-700 mb-2">
            Despesas Mensais (R$)
          </label>
          <div className="relative">
            <Car className="absolute left-3 top-3.5 text-slate-400" size={18} />
            <input
              type="number"
              name="monthlyExpenses"
              value={formData.monthlyExpenses}
              onChange={handleChange}
              className="w-full pl-10 pr-4 py-3 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
              placeholder="1500"
              min="0"
              required
            />
          </div>
        </div>

        {/* Tolerância ao Risco */}
        <div>
          <label className="block text-sm font-medium text-slate-700 mb-3">
            Tolerância ao Risco (1-10)
          </label>
          <div className="flex items-center gap-4">
            <TrendingUp className="text-slate-400" size={18} />
            <input
              type="range"
              name="riskTolerance"
              min="1"
              max="10"
              value={formData.riskTolerance}
              onChange={handleChange}
              className="flex-1 h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer slider"
            />
            <span className="text-lg font-bold text-indigo-600 min-w-[2rem]">
              {formData.riskTolerance}
            </span>
          </div>
          <div className="flex justify-between text-xs text-slate-500 mt-1">
            <span>Conservador</span>
            <span>Equilibrado</span>
            <span>Agressivo</span>
          </div>
        </div>

        {/* Localização */}
        <div>
          <label className="block text-sm font-medium text-slate-700 mb-2">
            Localização
          </label>
          <select
            name="location"
            value={formData.location}
            onChange={handleChange}
            className="w-full px-4 py-3 border border-slate-300 rounded-lg focus:ring-2 focus:ring-indigo-500 focus:border-indigo-500"
          >
            <option value="tampa-fl">Tampa, FL</option>
            <option value="sao-paulo-br">São Paulo, BR</option>
            <option value="rio-janeiro-br">Rio de Janeiro, BR</option>
            <option value="new-york-us">New York, US</option>
          </select>
        </div>

        {/* Botão Submit */}
        <button
          type="submit"
          disabled={isLoading}
          className="w-full bg-indigo-600 text-white font-bold py-4 px-6 rounded-xl hover:bg-indigo-700 disabled:bg-slate-400 disabled:cursor-not-allowed transition flex items-center justify-center gap-2"
        >
          {isLoading ? (
            <>
              <div className="animate-spin rounded-full h-5 w-5 border-b-2 border-white"></div>
              Calculando...
            </>
          ) : (
            <>
              <TrendingUp size={20} />
              Calcular Crédito Disponível
            </>
          )}
        </button>

        {/* Aviso */}
        <div className="flex items-start gap-2 p-3 bg-amber-50 border border-amber-200 rounded-lg">
          <AlertCircle className="text-amber-500 mt-0.5" size={16} />
          <p className="text-sm text-amber-700">
            Usamos simulação Monte Carlo com 10.000 iterações para calcular seu score baseado na previsibilidade de demanda.
          </p>
        </div>
      </form>
    </div>
  );
}