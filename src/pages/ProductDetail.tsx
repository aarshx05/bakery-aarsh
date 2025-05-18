import React, { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { ShoppingCart, Plus, Minus, ArrowLeft } from 'lucide-react';
import { getProductById, getProductsByCategory } from '../data/products';
import { ProductType } from '../types/types';
import { useCart } from '../context/CartContext';
import ProductCard from '../components/ui/ProductCard';

const ProductDetail: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const [product, setProduct] = useState<ProductType | null>(null);
  const [quantity, setQuantity] = useState(1);
  const [relatedProducts, setRelatedProducts] = useState<ProductType[]>([]);
  const { addToCart } = useCart();
  
  useEffect(() => {
    window.scrollTo(0, 0);
    
    if (id) {
      const foundProduct = getProductById(id);
      if (foundProduct) {
        setProduct(foundProduct);
        
        // Get related products from the same category
        const related = getProductsByCategory(foundProduct.category)
          .filter(p => p.id !== id)
          .slice(0, 4);
        setRelatedProducts(related);
      }
    }
  }, [id]);
  
  const handleQuantityChange = (newQuantity: number) => {
    if (newQuantity >= 1) {
      setQuantity(newQuantity);
    }
  };
  
  const handleAddToCart = () => {
    if (product) {
      addToCart(product, quantity);
    }
  };
  
  if (!product) {
    return (
      <div className="pt-28 pb-16 min-h-screen">
        <div className="container-custom">
          <p className="text-center text-lg">Product not found</p>
        </div>
      </div>
    );
  }

  return (
    <div className="pt-28 pb-16">
      <div className="container-custom">
        <Link to="/products" className="inline-flex items-center text-brown-600 hover:text-pink-500 mb-6">
          <ArrowLeft className="w-4 h-4 mr-1" /> Back to Products
        </Link>
        
        <div className="grid grid-cols-1 md:grid-cols-2 gap-10 mb-16">
          {/* Product Image */}
          <div className="rounded-lg overflow-hidden shadow-lg">
            <img 
              src={product.image} 
              alt={product.name} 
              className="w-full h-auto object-cover"
            />
          </div>
          
          {/* Product Info */}
          <div>
            <span className="text-sm font-medium text-pink-500 uppercase">
              {product.category.charAt(0).toUpperCase() + product.category.slice(1)}
            </span>
            <h1 className="text-3xl md:text-4xl font-playfair font-semibold text-brown-700 mt-2 mb-4">
              {product.name}
            </h1>
            <p className="text-2xl text-gold-600 font-semibold mb-6">
              ${product.price.toFixed(2)}
            </p>
            <div className="border-t border-b border-cream-300 py-6 mb-6">
              <p className="text-brown-600 leading-relaxed">{product.description}</p>
            </div>
            
            {/* Quantity Selector */}
            <div className="flex items-center mb-6">
              <span className="text-brown-700 mr-4">Quantity:</span>
              <div className="flex items-center border border-cream-300 rounded-md">
                <button
                  onClick={() => handleQuantityChange(quantity - 1)}
                  className="px-3 py-1 text-brown-600 hover:bg-cream-200 transition-colors"
                  disabled={quantity <= 1}
                >
                  <Minus className="w-4 h-4" />
                </button>
                <span className="px-4 py-1 border-l border-r border-cream-300">{quantity}</span>
                <button
                  onClick={() => handleQuantityChange(quantity + 1)}
                  className="px-3 py-1 text-brown-600 hover:bg-cream-200 transition-colors"
                >
                  <Plus className="w-4 h-4" />
                </button>
              </div>
            </div>
            
            {/* Add to Cart Button */}
            <button
              onClick={handleAddToCart}
              className="w-full btn btn-primary py-3 text-lg flex items-center justify-center gap-2"
            >
              <ShoppingCart className="w-5 h-5" /> Add to Cart
            </button>
          </div>
        </div>
        
        {/* Related Products */}
        {relatedProducts.length > 0 && (
          <div className="border-t border-cream-300 pt-12">
            <h2 className="text-2xl font-playfair font-semibold text-brown-700 mb-6">
              You May Also Like
            </h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-6">
              {relatedProducts.map((product) => (
                <ProductCard key={product.id} product={product} />
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default ProductDetail;