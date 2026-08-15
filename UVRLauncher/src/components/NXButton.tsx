import React from 'react';

interface NXButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: 'primary' | 'secondary' | 'danger' | 'ghost';
  size?: 'sm' | 'md' | 'lg';
  icon?: React.ReactNode;
}

export const NXButton: React.FC<NXButtonProps> = ({ 
  children, 
  variant = 'primary', 
  size = 'md', 
  icon,
  className = '',
  ...props 
}) => {
  const baseStyles = "relative flex items-center justify-center font-display font-semibold transition-all duration-300 rounded-lg overflow-hidden group";
  
  const variants = {
    primary: "bg-cyan/10 text-cyan border border-cyan/30 hover:bg-cyan hover:text-obsidian hover:shadow-[0_0_20px_rgba(0,245,212,0.6)] active:scale-95",
    secondary: "bg-surface text-white border border-white/10 hover:border-white/30 hover:bg-white/5 active:scale-95",
    danger: "bg-red-500/10 text-red-400 border border-red-500/30 hover:bg-red-500 hover:text-white hover:shadow-[0_0_20px_rgba(239,68,68,0.6)] active:scale-95",
    ghost: "text-gray-400 hover:text-white hover:bg-white/5 active:scale-95"
  };

  const sizes = {
    sm: "px-3 py-1.5 text-sm",
    md: "px-6 py-2.5 text-base",
    lg: "px-8 py-4 text-lg uppercase tracking-wider"
  };

  return (
    <button 
      className={`${baseStyles} ${variants[variant]} ${sizes[size]} ${className}`}
      {...props}
    >
      {/* 3D Depth element */}
      {variant === 'primary' && (
        <div className="absolute inset-0 bg-gradient-to-b from-white/20 to-transparent opacity-0 group-hover:opacity-100 transition-opacity"></div>
      )}
      
      {icon && <span className="mr-2 transition-transform group-hover:-translate-y-0.5">{icon}</span>}
      <span className="relative z-10">{children}</span>
    </button>
  );
};
