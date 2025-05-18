import React from 'react';
import { categories } from '../../data/products';
import CategoryCard from '../ui/CategoryCard';

const CategoriesSection: React.FC = () => {
  return (
    <section className="py-16">
      <div className="container-custom">
        <div className="text-center mb-12">
          <h2 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mb-3">
            Explore Our Categories
          </h2>
          <p className="text-brown-500 max-w-2xl mx-auto">
            From freshly baked breads to decadent cakes and our specialties
          </p>
        </div>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-8">
          {categories.map((category) => (
            <CategoryCard key={category.id} category={category} />
          ))}
        </div>
      </div>
    </section>
  );
};

export default CategoriesSection;
