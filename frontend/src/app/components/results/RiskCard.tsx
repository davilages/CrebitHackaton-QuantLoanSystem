import { Shield, AlertTriangle, CheckCircle } from "lucide-react";
import { Card, CardContent } from "../../ui/card";

interface RiskCardProps {
  riskScore: number;
  occupation: string;
  location: string;
}

export function RiskCard({ riskScore, occupation, location }: RiskCardProps) {
  const getRiskLevel = (score: number) => {
    if (score <= 20) return { label: "Low", color: "green", icon: CheckCircle };
    if (score <= 40) return { label: "Moderate", color: "yellow", icon: Shield };
    return { label: "High", color: "red", icon: AlertTriangle };
  };

  const risk = getRiskLevel(riskScore);
  const RiskIcon = risk.icon;

  return (
    <Card className={`bg-gradient-to-br from-${risk.color}-50 to-${risk.color}-100 border-${risk.color}-200`}>
      <CardContent className="p-6">
        <div className="flex items-start justify-between mb-4">
          <div>
            <p className={`text-xs font-semibold text-${risk.color}-700 uppercase tracking-wide mb-1`}>
              Risk Analysis
            </p>
            <h3 className={`text-3xl font-bold text-${risk.color}-900`}>
              {riskScore}
            </h3>
          </div>
          <div className={`w-12 h-12 bg-${risk.color}-200 rounded-xl flex items-center justify-center`}>
            <RiskIcon className={`w-6 h-6 text-${risk.color}-700`} />
          </div>
        </div>

        <div className="space-y-3">
          <div>
            <p className="text-xs text-slate-600 mb-1">Risk Level</p>
            <div className={`inline-block px-3 py-1 bg-${risk.color}-600 text-white text-sm font-bold rounded-lg`}>
              {risk.label}
            </div>
          </div>

          <div className="pt-3 border-t border-slate-200">
            <div className="space-y-2 text-sm">
              <div className="flex justify-between">
                <span className="text-slate-600">Occupation:</span>
                <span className="font-medium text-slate-900">{occupation}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-600">Location:</span>
                <span className="font-medium text-slate-900">{location}</span>
              </div>
            </div>
          </div>
        </div>
      </CardContent>
    </Card>
  );
}