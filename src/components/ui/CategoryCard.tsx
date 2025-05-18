import React from 'react';
import { Link } from 'react-router-dom';
import { CategoryType } from '../../types/types';

interface CategoryCardProps {
  category: CategoryType;
}

const CategoryCard: React.FC<CategoryCardProps> = ({ category }) => {
  return (
    <Link 
      to={`/products/${category.slug}`} 
      className="relative group overflow-hidden rounded-lg shadow-md"
    >
      <div className="aspect-w-4 aspect-h-3">
        <img 
          src={category.image} 
          alt={category.name} 
          className="w-full h-48 object-cover transition-transform duration-500 group-hover:scale-110"
        />
        <div className="absolute inset-0 bg-gradient-to-t from-black/70 to-transparent flex items-end justify-center p-4">
          <h3 className="text-white font-playfair text-2xl font-medium drop-shadow-md text-center">
            {category.name}
          </h3>
        </div>
      </div>
    </Link>
  );
};

export default CategoryCard;