import { TrendingUp, DollarSign, Percent, Calendar } from "lucide-react";
import { Card, CardContent } from "../../ui/card";

interface StatsGridProps {
  avgProfit: number;
  stdDev: number;
  confidenceLower: number;
  confidenceUpper: number;
}

export function StatsGrid({ avgProfit, stdDev, confidenceLower, confidenceUpper }: StatsGridProps) {
  const formatCurrency = (value: number) => {
    return new Intl.NumberFormat("en-US", {
      style: "currency",
      currency: "USD",
      minimumFractionDigits: 0
    }).format(value);
  };

  const stats = [
    {
      label: "Average Profit",
      value: formatCurrency(avgProfit),
      icon: DollarSign,
      color: "text-indigo-600"
    },
    {
      label: "Standard Deviation",
      value: formatCurrency(stdDev),
      icon: TrendingUp,
      color: "text-purple-600"
    },
    {
      label: "Lower Limit",
      value: formatCurrency(confidenceLower),
      icon: Percent,
      color: "text-blue-600"
    },
    {
      label: "Upper Limit",
      value: formatCurrency(confidenceUpper),
      icon: Calendar,
      color: "text-emerald-600"
    }
  ];

  return (
    <div className="grid grid-cols-2 gap-4">
      {stats.map((stat, index) => {
        const Icon = stat.icon;
        return (
          <Card key={index} className="hover:shadow-md transition-shadow">
            <CardContent className="p-4">
              <div className="flex items-center justify-between mb-3">
                <div className="w-10 h-10 bg-slate-100 rounded-lg flex items-center justify-center">
                  <Icon className={`w-5 h-5 ${stat.color}`} />
                </div>
              </div>
              <div>
                <p className="text-xs text-slate-600 mb-1">{stat.label}</p>
                <p className={`text-lg font-bold ${stat.color}`}>{stat.value}</p>
              </div>
            </CardContent>
          </Card>
        );
      })}
    </div>
  );
}