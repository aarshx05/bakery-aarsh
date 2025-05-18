import React from 'react';
import { Link } from 'react-router-dom';
import { ShoppingCart } from 'lucide-react';
import { ProductType } from '../../types/types';
import { useCart } from '../../context/CartContext';

interface ProductCardProps {
  product: ProductType;
}

const ProductCard: React.FC<ProductCardProps> = ({ product }) => {
  const { addToCart, cart } = useCart();
  
  const cartItem = cart.find(item => item.id === product.id);
  const quantityInCart = cartItem?.quantity || 0;

  const handleAddToCart = (e: React.MouseEvent) => {
    e.preventDefault();
    e.stopPropagation();
    addToCart(product);
  };

  return (
    <Link to={`/product/${product.id}`} className="product-card block">
      <div className="relative overflow-hidden group">
        <img 
          src={product.image} 
          alt={product.name} 
          className="w-full h-64 object-cover transition-transform duration-500 group-hover:scale-110"
        />
        <div className="absolute inset-0 bg-gradient-to-t from-black/60 to-transparent opacity-0 group-hover:opacity-100 transition-opacity duration-300 flex items-end">
          <div className="p-4 w-full">
            <button 
              onClick={handleAddToCart}
              className="w-full bg-pink-500 text-white py-2 rounded-md flex items-center justify-center space-x-2 hover:bg-pink-600 transition-colors"
            >
              <ShoppingCart className="w-5 h-5" />
              <span>Add to Cart</span>
            </button>
          </div>
        </div>
        {quantityInCart > 0 && (
          <div className="absolute top-2 right-2 bg-pink-500 text-white text-sm font-medium px-2 py-1 rounded-full">
            {quantityInCart} in cart
          </div>
        )}
      </div>
      <div className="p-4">
        <h3 className="font-playfair text-lg font-medium text-brown-700">{product.name}</h3>
        <p className="text-brown-500 mt-1 line-clamp-2 text-sm">{product.description}</p>
        <p className="text-gold-600 font-semibold mt-2">₹{product.price}</p>
      </div>
    </Link>
  );
};

export default ProductCard