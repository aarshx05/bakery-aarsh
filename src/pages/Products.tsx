import React, { useEffect, useState } from 'react';
import { useParams } from 'react-router-dom';
import ProductCard from '../components/ui/ProductCard';
import { products, categories, getProductsByCategory } from '../data/products';
import { ProductType } from '../types/types';

const Products: React.FC = () => {
  const { category } = useParams<{ category?: string }>();
  const [filteredProducts, setFilteredProducts] = useState<ProductType[]>([]);
  const [activeCategory, setActiveCategory] = useState<string>(category || 'all');
  
  useEffect(() => {
    window.scrollTo(0, 0);

    if (category) {
      setActiveCategory(category);
      setFilteredProducts(getProductsByCategory(category));
    } else if (activeCategory === 'all') {
      setFilteredProducts(products);
    } else {
      setFilteredProducts(getProductsByCategory(activeCategory));
    }
  }, [category, activeCategory]);
  
  const handleCategoryChange = (categorySlug: string) => {
    setActiveCategory(categorySlug);
    if (categorySlug === 'all') {
      setFilteredProducts(products);
    } else {
      setFilteredProducts(getProductsByCategory(categorySlug));
    }
  };

  return (
    <div className="pt-24 pb-16">
      <div className="container-custom">
        <div className="text-center mb-10">
          <h1 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mb-3">
            Our Products
          </h1>
          <p className="text-brown-500 max-w-2xl mx-auto">
            Explore our selection of freshly baked goods, made with love and the finest ingredients
          </p>
        </div>
        
        {/* Category Filters */}
        <div className="flex flex-wrap justify-center gap-2 mb-10">
          <button
            onClick={() => handleCategoryChange('all')}
            className={`px-4 py-2 rounded-full text-sm font-medium transition-colors ${
              activeCategory === 'all'
                ? 'bg-pink-500 text-white'
                : 'bg-cream-200 text-brown-600 hover:bg-cream-300'
            }`}
          >
            All Products
          </button>
          {categories.map((cat) => (
            <button
              key={cat.id}
              onClick={() => handleCategoryChange(cat.slug)}
              className={`px-4 py-2 rounded-full text-sm font-medium transition-colors ${
                activeCategory === cat.slug
                  ? 'bg-pink-500 text-white'
                  : 'bg-cream-200 text-brown-600 hover:bg-cream-300'
              }`}
            >
              {cat.name}
            </button>
          ))}
        </div>
        
        {/* Products Grid */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-6">
          {filteredProducts.map((product) => (
            <ProductCard key={product.id} product={product} />
          ))}
        </div>
        
        {filteredProducts.length === 0 && (
          <div className="text-center py-10">
            <p className="text-lg text-brown-600">No products found in this category.</p>
          </div>
        )}
      </div>
    </div>
  );
};

export default Products;